/*
*		File: Cam_Func.c
*		Purpose: The source file containing functions to provide V4L2 interface
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*
*		Most the functions in this source file have been developed with reference to the
*		code developed by Professor Sam Siewert. The source code can be found in the below link.
*		http://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/computer-vision/simple-capture/
*
*/

#include "main.h"
#include "Aux_Func.h"
#include "Cam_Func.h"

// Shared Variables
char *dev_name;
int fd;
struct v4l2_format fmt;
frame_p_buffer *frame_p;
uint32_t n_buffers;
int force_format;
uint32_t HRES, VRES;
strct_analyze Analysis;

// Local Variables
static float Warmup_Stamp_1, Warmup_Stamp_2;
static int resp;

// Declare set to use with FD_macros
static fd_set fds;

// Structure to store timing values
static struct timeval tout;

// Function to poll the camera for new frame, and simply discard it
// Parameter1: void
// Return: uint8_t result - 0: success
static uint8_t get_discard_frame(void)
{
	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buffer
	struct v4l2_buffer buf;

	// Set the entire structure to 0
	CLEAR(buf);

	// Set proper type
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Indicate that the method being used is memory mapping
	buf.memory = V4L2_MEMORY_MMAP;

	// Dequeue a filled buffer from driver's outgoing queue
	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
	{
		switch (errno)
		{
			case EAGAIN:
			{
				return 1;
			}

			case EIO:
			/* Could ignore EIO, see spec. */

			/* fall through */

			default:
			{
				syslog(LOG_ERR, "<%.6fms>!!Cam_Func!! VIDIOC_DQBUF Error", Time_Stamp(Mode_ms));

				errno_exit("VIDIOC_DQBUF");
			
				break;
			}
		}
	}

	assert(buf.index < n_buffers);

	// Enqueue back the empty buffer to driver's incoming queue
	if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Func!! VIDIOC_QBUF Error", Time_Stamp(Mode_ms));

		errno_exit("VIDIOC_QBUF");
	}

	return 0;
}

// Function to warmup the camera by capturing and discarding a few initial frames
// And to also calculate maximum FPS
// Parameter1: void
// Return: uint8_t result - 0: success
uint8_t device_warmup(void)
{
	uint32_t tmp = 0;

	Warmup_Stamp_1 = Time_Stamp(Mode_ms);

	while(tmp < Warmup_Frames)
	{

		// Clear the set
		FD_ZERO(&fds);

		// Store file descriptor of camera in the set
		FD_SET(fd, &fds);

		// Set timeout of communication with Camera to 2 seconds
		tout.tv_sec = Warmup_Select_Timeout_s;
		tout.tv_usec = 0;

		// http://man7.org/linux/man-pages/man2/select.2.html
		// Select() blockingly monitors given file descriptors for their availibity for I/O operations
		// First parameter: nfds should be set to the highest-numbered file descriptor in any of the three sets, plus 1
		// Second parameter: file drscriptor set that should be monitored for read() availability
		// Third parameter: file drscriptor set that should be monitored for write() availability
		// Fourth parameter: file drscriptor set that should be monitored for exceptions
		// Fifth parameter: to specify time out for select()
		resp = select(fd + 1, &fds, NULL, NULL, &tout);

		// Error handling
		if(resp == -1)
		{
			// If a signal made select() to return, then skip rest of the loop code, and start again
			if(EINTR == errno)
			{
				return 1;
			}

			syslog(LOG_ERR, "<%.6fms>!!Cam_Func!! Select() Error", Time_Stamp(Mode_ms));

			errno_exit("select");
		}

		// 0 return means that no file descriptor made the select() to close
		else if(resp == 0)
		{

			syslog(LOG_ERR, "<%.6fms>!!Cam_Func!! Select() Timedout", Time_Stamp(Mode_ms));

			errno_exit("select timeout");
		}

		// If select() indicated that device is ready for reading from it, then grab a frame
		else if(get_discard_frame() != 0)
		{
			Analysis.Max_FPS = 0;
			return 1;
		}

		tmp += 1;

	}

	Warmup_Stamp_2 = Time_Stamp(Mode_ms);

	Analysis.Max_FPS = (Warmup_Frames * (float)1000) / (Warmup_Stamp_2 - Warmup_Stamp_1);

	return 0;
}

// Function to exit the program on failure
// Parameter1: const char *s - string to print while exiting
// Return: void
void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
//        pthread_exit(0);
        exit(EXIT_FAILURE);
}

// Function to provide a do while wrapper on ioctl
// Parameters: same as ioctl parameters
// Return: int result - Non negative integer on success
int xioctl(int fh, int request, void *arg)
{
        int r;

        do
        {
            r = ioctl(fh, request, arg);

        } while (-1 == r && EINTR == errno);

        return r;
}

// Function to tell driver using V4L2 to turn off streaming
// Parameter1: void
// Return: void
void stop_capturing(void)
{
        enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
	{
		errno_exit("VIDIOC_STREAMOFF");
	}
}

// Function to configure driver using V4L2 to start streaming in memory mapping mode
// Parameter1: void
// Return: void
void start_capturing(void)
{
        uint32_t i;

        enum v4l2_buf_type type;

        for (i = 0; i < n_buffers; i++)
	{

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buffer
		struct v4l2_buffer buf;

		// Set the entire structure to 0
		CLEAR(buf);

		// Set proper buffer type
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		// Indicate that the method is memory mapped I/O
		buf.memory = V4L2_MEMORY_MMAP;

		// Setting buffer index
		buf.index = i;

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#vidioc-qbuf
		// To enqueue an empty buffer in the driver's incoming queue
		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		{
			errno_exit("VIDIOC_QBUF");
		}
        }

	// Set the type to buffer video input
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Start streaming I/O
	if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
	{
		errno_exit("VIDIOC_STREAMON");
	}
}

// Function to unmap the previously mapped region on memory
// Parameter1: void
// Return: void
void uninit_device(void)
{
        uint32_t i;

        for (i = 0; i < n_buffers; i++)
	{
		if (-1 == munmap(frame_p[i].start, frame_p[i].length))
		{
			errno_exit("munmap");
		}
	}

        free(frame_p);
}

// Function to properly initialize memory mapping
// Parameter1: void
// Return: void
void init_mmap(uint32_t buffer_size)
{
	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-requestframe_p
	// Required structure for initiating memory mapping or user pointer I/O
	struct v4l2_requestbuffers req;

	// Set the entire structure to 0
	CLEAR(req);

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-requestframe_p
	// Request no of buffers as defined in the header file
	req.count  = MMAP_Buffers_Count;

	// Set the proper type
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Indicate that the method I/O is memory map
	req.memory = V4L2_MEMORY_MMAP;

	// Switch the driver into memory mapped I/O mode
	if(-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
	{
		if(EINVAL == errno)
		{
			fprintf(stderr, "%s does not support user pointer i/o\n", dev_name);
			exit(EXIT_FAILURE);
		} 
		
		else
		{
			errno_exit("VIDIOC_REQBUFS");
		}
	}

        if (req.count < MMAP_Buffers_Count)
        {
                fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
                exit(EXIT_FAILURE);
        }

	// Allocate 4 (arbitarily selected number I think) blocks of memory, for bufffers that has bookkeeping data of actual image frame_p
	frame_p = calloc(req.count, sizeof(*frame_p));

	// Error handling
	if(!frame_p)
	{
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	// Dynamically allocate memory for each of these frame_p
	for(n_buffers = 0; n_buffers < req.count; n_buffers++)
	{
		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buffer
                struct v4l2_buffer buf;

				// Set entire structure to 0
                CLEAR(buf);

				// Set proper type
                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                // https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-memory
                // Indicate that this buffer will be used for memory mapping I/O
                buf.memory      = V4L2_MEMORY_MMAP;

                // Set buffer indexes properly
                buf.index       = n_buffers;

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#vidioc-querybuf
		// Query the status of given buffer
                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
		{
                        errno_exit("VIDIOC_QUERYBUF");
		}

				// Set required length to the size of the buffer (not payload)
                frame_p[n_buffers].length = buf.length;

                // http://man7.org/linux/man-pages/man2/mmap.2.html
                // First parameter: If addr is NULL, then the kernel chooses the (page-aligned) address at which to create the mapping; this is the most portable method of creating a new mapping
                // Second parameter: Length (number of bytes) to be allocated
                // Third parameter: The allocated page(s) may be read as well as written
                // Fourth parameter: Share this mapping. Visible to other processes mapping the same region
                // Fifth parameter: File Descriptor for the camera
                // Sixth parameter: Offset of the buffer from the start of the device memory
                frame_p[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                // Error handling
                if (MAP_FAILED == frame_p[n_buffers].start)
		{
                        errno_exit("mmap");exit(EXIT_FAILURE);
		}
	}
}

// Function to initialize camera and the driver with appropriate settings, using V4L2
// Parameter1: void
// Return: void
void init_device(void)
{
	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-capability
	// Device capabilities structure
	struct v4l2_capability cap;

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-cropcap
	// Video cropping and scaling abilities structure
	struct v4l2_cropcap cropcap;

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-crop
	// Current cropping rectangle structure
	struct v4l2_crop crop;
	uint32_t min;

	// Query device capabilities, using wrapper of ioctl
	// If 0 is returned then the V4L2 supports device
	if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
	{
		if (EINVAL == errno)
		{
			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
			exit(EXIT_FAILURE);
		}
		else
		{
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#capture
	// Check whether the device supports the video capture interface with V4L2
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#rw
	// Based on the desired mode of operation, check whether the device
	// supports read and streaming

	if(!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
		exit(EXIT_FAILURE);
	}



	/* Select video input, video standard and tune here. */

	// Set the entire structure to 0
	CLEAR(cropcap);

	// Seeting buffer type to buffer of a video capture stream
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#vidioc-cropcap
	// Proceed if querying information about the video cropping and scaling abilities succeeds
	if(0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
	{
		// Select the appropriate buffer
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-cropcap
		// Set the cropping rectangle to the default one
		crop.c = cropcap.defrect;

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#vidioc-g-crop
		// Open file to update the cropping parameters
		if(-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
		{
			switch(errno)
			{
				case EINVAL:
				/* Cropping not supported. */
				break;
				default:
				/* Errors ignored. */
				break;
			}
		}

	}

	else
	{
		/* Errors ignored. */
	}

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-format
	// Clear format structure - set to all 0
	CLEAR(fmt);

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buf-type
	// Set type to buffer video capture
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Override default formatting if specified
	if(force_format)
	{
		printf("FORCING FORMAT\n");

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-pix-format
		// Set image width and height in pixels
		fmt.fmt.pix.width       = HRES;
		fmt.fmt.pix.height      = VRES;

		// Specify the Pixel Coding Format here

		// http://wiki.oz9aec.net/index.php/Logitech_HD_Pro_Webcam_C920
		// TODO: Try other compressed formats listed above
		// This one work for Logitech C920
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

		//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
		//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_VYUY;

		// Would be nice if camera supported
		// TODO: Try GREY scale one on the given camera
		//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
		//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

		//fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-field
		// Images are in progressive format, not interlaced
		fmt.fmt.pix.field       = V4L2_FIELD_NONE;

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#vidioc-g-fmt
		// Set the data format
		if(-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
		{
			errno_exit("VIDIOC_S_FMT");
		}

		/* Note VIDIOC_S_FMT may change width and height. */
	}
	else
	{
		printf("ASSUMING FORMAT\n");
		/* Preserve original settings as set by v4l2-ctl for example */
		if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
		{
			errno_exit("VIDIOC_G_FMT");
		}
	}

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-pix-format
	// Looks like these parameters should be set automatically, however, as it says before, driver could be buggy
	// Buggy driver paranoia.
	min = fmt.fmt.pix.width * 2;
	if(fmt.fmt.pix.bytesperline < min)
	{
		fmt.fmt.pix.bytesperline = min;
		min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	}

	if(fmt.fmt.pix.sizeimage < min)
	{
		fmt.fmt.pix.sizeimage = min;
	}

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#userp
	// Call the proper function to setup user pointer background - that is specifically required
	// Pass the size of the one frame as function parameter
	init_mmap(fmt.fmt.pix.sizeimage);
}

// Function to close the file handler of camera
// Parameter1: void
// Return: void
void close_device(void)
{
        if (-1 == close(fd))
	{
		errno_exit("close");
	}
        fd = -1;
}

// Function to start communicating with camera, and to get a file handler for same
// Parameter1: void
// Return: void
void open_device(void)
{

	// http://man7.org/linux/man-pages/man2/stat.2.html

	struct stat st;

	// Retrieve information about the file pointed to by dev_name
	// Also, store the same in st structure
	// Error handling
	if (-1 == stat(dev_name, &st))
	{
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
		dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// https://www.gnu.org/software/libc/manual/html_node/Testing-File-Type.html
	// This macro returns non-zero if the file is a character special file (a device like a terminal)
	if (!S_ISCHR(st.st_mode))
	{
		fprintf(stderr, "%s is no device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	// Get access to camera
	fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	// Error handling
	if (-1 == fd)
	{
		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
