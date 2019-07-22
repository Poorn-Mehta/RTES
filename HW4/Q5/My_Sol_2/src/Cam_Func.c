// Cam_Func.c

#include "main.h"

extern char *dev_name;
extern int fd;
struct v4l2_format fmt;
extern frame_p_buffer *frame_p;
extern uint32_t n_buffers;
extern int force_format;

extern uint32_t HRES, VRES;

void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
//        pthread_exit(0);
        exit(EXIT_FAILURE);
}

int xioctl(int fh, int request, void *arg)
{
        int r;

        do
        {
            r = ioctl(fh, request, arg);

        } while (-1 == r && EINTR == errno);

        return r;
}

void stop_capturing(void)
{
        enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
	{
		errno_exit("VIDIOC_STREAMOFF");
	}
}

void start_capturing(void)
{
        uint32_t i;

        enum v4l2_buf_type type;

        for (i = 0; i < n_buffers; ++i)
	{

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buffer
		struct v4l2_buffer buf;

		// Set the entire structure to 0
		CLEAR(buf);

		// Set proper buffer type
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		// Indicate that the method is user pointer I/O
		buf.memory = V4L2_MEMORY_USERPTR;

		// Setting buffer index
		buf.index = i;

		// Loading pointer to the user buffer in the structure that will be passed to the driver
		buf.m.userptr = (unsigned long)frame_p[i].start;

		// Loading length of the user buffer
		buf.length = frame_p[i].length;

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

void uninit_device(void)
{
        uint32_t i;

        for (i = 0; i < n_buffers; ++i)
	{
		free(frame_p[i].start);
	}

        free(frame_p);
}

void init_userp(uint32_t buffer_size)
{
	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-requestframe_p
	// Required structure for initiating memory mapping or user pointer I/O
	struct v4l2_requestbuffers req;

	// Set the entire structure to 0
	CLEAR(req);

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-requestframe_p
	// TODO: Comment out this field. According to the documentation, this is utterly useless.
	req.count  = 4;

	// Set the proper type
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Indicate that the method I/O is user pointer
	req.memory = V4L2_MEMORY_USERPTR;

	// Switch the driver into user pointer I/O mode
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

	// Allocate 4 (arbitarily selected number I think) blocks of memory, for bufffers that has bookkeeping data of actual image frame_p
	frame_p = calloc(4, sizeof(*frame_p));

	// Error handling
	if(!frame_p)
	{
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	// Dynamically allocate memory for each of these frame_p
	for(n_buffers = 0; n_buffers < 4; ++n_buffers)
	{
		frame_p[n_buffers].length = buffer_size;
		frame_p[n_buffers].start = malloc(buffer_size);

		if (!frame_p[n_buffers].start)
		{
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

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
	/* Buggy driver paranoia. */
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
	init_userp(fmt.fmt.pix.sizeimage);
}


void close_device(void)
{
        if (-1 == close(fd))
	{
		errno_exit("close");
	}
        fd = -1;
}

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
