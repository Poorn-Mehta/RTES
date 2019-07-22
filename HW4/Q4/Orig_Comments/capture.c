/*
 *
 *  Adapted by Sam Siewert for use with UVC web cameras and Bt878 frame
 *  grabber NTSC cameras to acquire digital video from a source,
 *  time-stamp each frame acquired, save to a PGM or PPM file.
 *
 *  The original code adapted was open source from V4L2 API and had the
 *  following use and incorporation policy:
 *
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see http://linuxtv.org/docs.php for more information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

// V4L2 Library
#include <linux/videodev2.h>

#include <time.h>

// To set passed variable to 0
#define CLEAR(x) memset(&(x), 0, sizeof(x))
//#define COLOR_CONVERT

// Resolution Defines and Strings
//#define HRES 320
//#define VRES 240
//#define HRES_STR "320"
//#define VRES_STR "240"
#define HRES 800
#define VRES 600
#define HRES_STR "800"
#define VRES_STR "600"

// Format is used by a number of functions, so made as a file global
// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-format
static struct v4l2_format fmt;

enum io_method
{
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};

struct buffer
{
        void   *start;
        size_t  length;
};

static char            *dev_name;
static enum io_method   io = IO_METHOD_USERPTR;
//static enum io_method   io = IO_METHOD_READ;
//static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer          *buffers;
static unsigned int     n_buffers;
static int              out_buf;
static int              force_format=1;
static int              frame_count = 30;

static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
        int r;

        do
        {
            r = ioctl(fh, request, arg);

        } while (-1 == r && EINTR == errno);

        return r;
}

char ppm_header[]="P6\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char ppm_dumpname[]="img/test00000000.ppm";

static void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time)
{
    int written, i, total, dumpfd;

    snprintf(&ppm_dumpname[8], 9, "%08d", tag);
    strncat(&ppm_dumpname[16], ".ppm", 5);
    dumpfd = open(ppm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);

    snprintf(&ppm_header[4], 11, "%010d", (int)time->tv_sec);
    strncat(&ppm_header[14], " sec ", 5);
    snprintf(&ppm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
    strncat(&ppm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);
    written=write(dumpfd, ppm_header, sizeof(ppm_header));

    total=0;

    do
    {
        written=write(dumpfd, p, size);
        total+=written;
    } while(total < size);

    printf("wrote %d bytes\n", total);

    close(dumpfd);

}


char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char pgm_dumpname[]="img/test00000000.pgm";

static void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time)
{
    int written, i, total, dumpfd;

   	// Set name in a way that it reflects the frame number
    snprintf(&pgm_dumpname[8], 9, "%08d", tag);

    // Append .pgm at the last
    strncat(&pgm_dumpname[16], ".pgm", 5);

    // Create the file
    dumpfd = open(pgm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);

	// Store timestamp and resolution in the header
    snprintf(&pgm_header[4], 11, "%010d", (int)time->tv_sec);
    strncat(&pgm_header[14], " sec ", 5);
    snprintf(&pgm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
    strncat(&pgm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);

    // Write header in file
    written=write(dumpfd, pgm_header, sizeof(pgm_header));

    total=0;

	// Write the entire buffer having the image information to the file
    do
    {
        written=write(dumpfd, p, size);
        total+=written;
    } while(total < size);

    printf("wrote %d bytes\n", total);

	// Close file
    close(dumpfd);

}


void yuv2rgb_float(float y, float u, float v,
                   unsigned char *r, unsigned char *g, unsigned char *b)
{
    float r_temp, g_temp, b_temp;

    // R = 1.164(Y-16) + 1.1596(V-128)
    r_temp = 1.164*(y-16.0) + 1.1596*(v-128.0);
    *r = r_temp > 255.0 ? 255 : (r_temp < 0.0 ? 0 : (unsigned char)r_temp);

    // G = 1.164(Y-16) - 0.813*(V-128) - 0.391*(U-128)
    g_temp = 1.164*(y-16.0) - 0.813*(v-128.0) - 0.391*(u-128.0);
    *g = g_temp > 255.0 ? 255 : (g_temp < 0.0 ? 0 : (unsigned char)g_temp);

    // B = 1.164*(Y-16) + 2.018*(U-128)
    b_temp = 1.164*(y-16.0) + 2.018*(u-128.0);
    *b = b_temp > 255.0 ? 255 : (b_temp < 0.0 ? 0 : (unsigned char)b_temp);
}


// This is probably the most acceptable conversion from camera YUYV to RGB
//
// Wikipedia has a good discussion on the details of various conversions and cites good references:
// http://en.wikipedia.org/wiki/YUV
//
// Also http://www.fourcc.org/yuv.php
//
// What's not clear without knowing more about the camera in question is how often U & V are sampled compared
// to Y.
//
// E.g. YUV444, which is equivalent to RGB, where both require 3 bytes for each pixel
//      YUV422, which we assume here, where there are 2 bytes for each pixel, with two Y samples for one U & V,
//              or as the name implies, 4Y and 2 UV pairs
//      YUV420, where for every 4 Ys, there is a single UV pair, 1.5 bytes for each pixel or 36 bytes for 24 pixels

void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b)
{
   int r1, g1, b1;

   // replaces floating point coefficients
   int c = y-16, d = u - 128, e = v - 128;

   // Conversion that avoids floating point
   r1 = (298 * c           + 409 * e + 128) >> 8;
   g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
   b1 = (298 * c + 516 * d           + 128) >> 8;

   // Computed values may need clipping.
   if (r1 > 255) r1 = 255;
   if (g1 > 255) g1 = 255;
   if (b1 > 255) b1 = 255;

   if (r1 < 0) r1 = 0;
   if (g1 < 0) g1 = 0;
   if (b1 < 0) b1 = 0;

   *r = r1 ;
   *g = g1 ;
   *b = b1 ;
}



unsigned int framecnt=0;
unsigned char bigbuffer[(1280*960)];

static void process_image(const void *p, int size)
{
    int i, newi, newsize=0;
    struct timespec frame_time;
    int y_temp, y2_temp, u_temp, v_temp;
    unsigned char *pptr = (unsigned char *)p;

    // Absolute timestamp - store the current time in structure
    clock_gettime(CLOCK_REALTIME, &frame_time);

	// Increase framecount by 1
    framecnt++;
    printf("frame %d: ", framecnt);

	// If the captured image is already in grayscale, then simply store it in proper format
    if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY)
    {
        printf("Dump graymap as-is size %d\n", size);
        dump_pgm(p, size, framecnt, &frame_time);
    }

	// Handle the required image processing for YUYV capture
    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {

#if defined(COLOR_CONVERT)
        printf("Dump YUYV converted to RGB size %d\n", size);

        // Pixels are YU and YV alternating, so YUYV which is 4 bytes
        // We want RGB, so RGBRGB which is 6 bytes
        //
        for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
        {
            y_temp=(int)pptr[i]; u_temp=(int)pptr[i+1]; y2_temp=(int)pptr[i+2]; v_temp=(int)pptr[i+3];
            yuv2rgb(y_temp, u_temp, v_temp, &bigbuffer[newi], &bigbuffer[newi+1], &bigbuffer[newi+2]);
            yuv2rgb(y2_temp, u_temp, v_temp, &bigbuffer[newi+3], &bigbuffer[newi+4], &bigbuffer[newi+5]);
        }

        dump_ppm(bigbuffer, ((size*6)/4), framecnt, &frame_time);
#else
        printf("Dump YUYV converted to YY size %d\n", size);

        // Pixels are YU and YV alternating, so YUYV which is 4 bytes
        // We want Y, so YY which is 2 bytes
        //
        for(i=0, newi=0; i<size; i=i+4, newi=newi+2)
        {
            // Y1=first byte and Y2=third byte
            bigbuffer[newi]=pptr[i];
            bigbuffer[newi+1]=pptr[i+2];
        }

        dump_pgm(bigbuffer, (size/2), framecnt, &frame_time);
#endif

    }

	// If the captured image is already in RGB, then simply store it in proper format
    else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24)
    {
        printf("Dump RGB as-is size %d\n", size);
        dump_ppm(p, size, framecnt, &frame_time);
    }

    // Error handling
    else
    {
        printf("ERROR - unknown dump format\n");
    }

    fflush(stderr);
    //fprintf(stderr, ".");
    fflush(stdout);
}


static int read_frame(void)
{
	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buffer
    struct v4l2_buffer buf;

    unsigned int i;

    switch (io)
    {

		// In case of simple reading, read the raw image (YUYV format) from camera
		// Store it at malloced space using buffer that has pointer and length of the same
        case IO_METHOD_READ:
            if (-1 == read(fd, buffers[0].start, buffers[0].length))
            {
                switch (errno)
                {

                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        errno_exit("read");
                }
            }

			// Process image if there was no error in reading
            process_image(buffers[0].start, buffers[0].length);
            break;

        case IO_METHOD_MMAP:

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
                        return 0;

                    case EIO:
                        /* Could ignore EIO, but drivers should only set for serious errors, although some set for
                           non-fatal errors too.
                         */
                        return 0;


                    default:
                        printf("mmap failure\n");
                        errno_exit("VIDIOC_DQBUF");
                }
            }

			// Ensure that the buffer received is a valid one
            assert(buf.index < n_buffers);

			// Process the image
            process_image(buffers[buf.index].start, buf.bytesused);

			// Enqueue back the empty buffer to driver's incoming queue
            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
            break;

        case IO_METHOD_USERPTR:

        	// Set the entire structure to 0
            CLEAR(buf);

			// Set proper type
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            // Indicate that the method being used is memory mapping
            buf.memory = V4L2_MEMORY_USERPTR;

			// Dequeue a filled buffer from driver's outgoing queue
            if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
            {
                switch (errno)
                {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        errno_exit("VIDIOC_DQBUF");
                }
            }

			// Ensure that the buffer received is a valid one
            for (i = 0; i < n_buffers; ++i)
                    if (buf.m.userptr == (unsigned long)buffers[i].start
                        && buf.length == buffers[i].length)
                            break;

            assert(i < n_buffers);

			// Process the image
            process_image((void *)buf.m.userptr, buf.bytesused);

			// Enqueue back the empty buffer to driver's incoming queue
            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
            break;
    }

    //printf("R");
    return 1;
}


static void mainloop(void)
{
    unsigned int count;

    // Structures to store time
    struct timespec read_delay;
    struct timespec time_error;

	// Arbitarily selected time of 30us
    read_delay.tv_sec=0;
    read_delay.tv_nsec=30000;

	// Set how many frames are going to be captured
    count = frame_count;

	// Keep on executing till all frames have been captured
	// TODO: Remove this while - I think that it's pointless
    while (count > 0)
    {
        for (;;)
        {
        	// Declare set to use with FD_macros
            fd_set fds;

            // Structure to store timing values
            struct timeval tv;

            int r;

			// Clear the set
            FD_ZERO(&fds);

            // Store file descriptor of camera in the set
            FD_SET(fd, &fds);

            // Set timeout of communication with Camera to 2 seconds
            tv.tv_sec = 2;
            tv.tv_usec = 0;

			// http://man7.org/linux/man-pages/man2/select.2.html
			// Select() blockingly monitors given file descriptors for their availibity for I/O operations
			// First parameter: nfds should be set to the highest-numbered file descriptor in any of the three sets, plus 1
			// Second parameter: file drscriptor set that should be monitored for read() availability
			// Third parameter: file drscriptor set that should be monitored for write() availability
			// Fourth parameter: file drscriptor set that should be monitored for exceptions
			// Fifth parameter: to specify time out for select()
            r = select(fd + 1, &fds, NULL, NULL, &tv);

			// Error handling
            if (-1 == r)
            {
            	// If a signal made select() to return, then skip rest of the loop code, and start again
                if (EINTR == errno)
                    continue;
                errno_exit("select");
            }

			// 0 return means that no file descriptor made the select() to close
            if (0 == r)
            {
                fprintf(stderr, "select timeout\n");
                exit(EXIT_FAILURE);
            }

			// If select() indicated that device is ready for reading from it, then grab a frame
            if (read_frame())
            {
            	// Frame reading succeede, sleep for predetermined amount of time
                if(nanosleep(&read_delay, &time_error) != 0)
                    perror("nanosleep");
                else
                    printf("time_error.tv_sec=%ld, time_error.tv_nsec=%ld\n", time_error.tv_sec, time_error.tv_nsec);

				// Indicate that one valid frame capturing has been completed
                count--;
                break;
            }

            /* EAGAIN - continue select loop unless count done. */
            if(count <= 0) break;
        }

        if(count <= 0) break;
    }
}

static void stop_capturing(void)
{
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
                        errno_exit("VIDIOC_STREAMOFF");
                break;
        }
}

static void start_capturing(void)
{
        unsigned int i;

        enum v4l2_buf_type type;

        switch (io)
        {

        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i)
                {
                        printf("allocated buffer %d\n", i);

                        // https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buffer
                        struct v4l2_buffer buf;

						// Set the entire structure to 0
                        CLEAR(buf);

                        // Set proper buffer type
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                        // Indicate that the method is memory mapping I/O
                        buf.memory = V4L2_MEMORY_MMAP;

                        // Setting buffer index
                        buf.index = i;

						// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#vidioc-qbuf
						// To enqueue an empty buffer in the driver's incoming queue
                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }

                // Set the type to buffer video input
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                // Start streaming I/O
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i) {

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
                        buf.m.userptr = (unsigned long)buffers[i].start;

                        // Loading length of the user buffer
                        buf.length = buffers[i].length;

						// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#vidioc-qbuf
						// To enqueue an empty buffer in the driver's incoming queue
                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }

                 // Set the type to buffer video input
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                // Start streaming I/O
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;
        }
}

static void uninit_device(void)
{
        unsigned int i;

        switch (io) {
        case IO_METHOD_READ:
                free(buffers[0].start);
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i)
                        if (-1 == munmap(buffers[i].start, buffers[i].length))
                                errno_exit("munmap");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i)
                        free(buffers[i].start);
                break;
        }

        free(buffers);
}

static void init_read(unsigned int buffer_size)
{
		// https://www.geeksforgeeks.org/calloc-versus-malloc/
		// Allocate one block, and initialize it with 0
		// This block will only contain pointer to the buffer where actual image is
		// and length of that buffer
        buffers = calloc(1, sizeof(*buffers));

		// Error handling
        if (!buffers)
        {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

		// Set length to the size of every image
        buffers[0].length = buffer_size;

        // Set the starting pointer, and allocate memory for the buffer
        buffers[0].start = malloc(buffer_size);

		// Error handling
        if (!buffers[0].start)
        {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }
}

static void init_mmap(void)
{
		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-requestbuffers
		// Required structure for initiating memory mapping or user pointer I/O
        struct v4l2_requestbuffers req;

		// Set the entire structure to 0
        CLEAR(req);

		// Request 6 buffers
		// TODO: Find out why 6
		// I think it could be larger, based on the video memory of GPU - or something that's device memory for the driver
        req.count = 6;

        // https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buf-type
        // Set the buffered video input type
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        // Set to memory mapping
        req.memory = V4L2_MEMORY_MMAP;

		// Request memory mapping mode with the specifics (parameters set)
        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
        {
                if (EINVAL == errno)
                {
                        fprintf(stderr, "%s does not support "
                                 "memory mapping\n", dev_name);
                        exit(EXIT_FAILURE);
                } else
                {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

		// If got less than 2 buffers - that is only 1 or 0, then give out error
        if (req.count < 2)
        {
                fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
                exit(EXIT_FAILURE);
        }

		// Allocate one block of memory, per allocated buffer in device memory
		// Each of this block will hold length and starting pointer to the data
        buffers = calloc(req.count, sizeof(*buffers));

		// Error handling
        if (!buffers)
        {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

		// Before applications can access the buffers they must map them into their address space with the mmap() function
		// TODO: Check whether following line should be ++n_buffers or n_buffers++
		// I think it should be latter one based on this information (source: officlal documentation)
		// Number of the buffer, set by the application. This field is only used for memory mapping I/O and can range from zero to the number of buffers allocated with the VIDIOC_REQBUFS ioctl (struct v4l2_requestbuffers count) minus one
        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {

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
                        errno_exit("VIDIOC_QUERYBUF");

				// Set required length to the size of the buffer (not payload)
                buffers[n_buffers].length = buf.length;

                // http://man7.org/linux/man-pages/man2/mmap.2.html
                // First parameter: If addr is NULL, then the kernel chooses the (page-aligned) address at which to create the mapping; this is the most portable method of creating a new mapping
                // Second parameter: Length (number of bytes) to be allocated
                // Third parameter: The allocated page(s) may be read as well as written
                // Fourth parameter: Share this mapping. Visible to other processes mapping the same region
                // Fifth parameter: File Descriptor for the camera
                // Sixth parameter: Offset of the buffer from the start of the device memory
                buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                // Error handling
                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}

static void init_userp(unsigned int buffer_size)
{
		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-requestbuffers
		// Required structure for initiating memory mapping or user pointer I/O
        struct v4l2_requestbuffers req;

		// Set the entire structure to 0
        CLEAR(req);

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-requestbuffers
		// TODO: Comment out this field. According to the documentation, this is utterly useless.
        req.count  = 4;

        // Set the proper type
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        // Indicate that the method I/O is user pointer
        req.memory = V4L2_MEMORY_USERPTR;

		// Switch the driver into user pointer I/O mode
        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "user pointer i/o\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

		// Allocate 4 (arbitarily selected number I think) blocks of memory, for bufffers that has bookkeeping data of actual image buffers
        buffers = calloc(4, sizeof(*buffers));

		// Error handling
        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

		// Dynamically allocate memory for each of these buffers
        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = malloc(buffer_size);

                if (!buffers[n_buffers].start) {
                        fprintf(stderr, "Out of memory\n");
                        exit(EXIT_FAILURE);
                }
        }
}

static void init_device(void)
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
    unsigned int min;

	// Query device capabilities, using wrapper of ioctl
	// If 0 is returned then the V4L2 supports device
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n",
                     dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
                errno_exit("VIDIOC_QUERYCAP");
        }
    }

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#capture
	// Check whether the device supports the video capture interface with V4L2
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%s is no video capture device\n",
                 dev_name);
        exit(EXIT_FAILURE);
    }

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#rw
	// Based on the desired mode of operation, check whether the device
	// supports read and streaming
    switch (io)
    {
        case IO_METHOD_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE))
            {
                fprintf(stderr, "%s does not support read i/o\n",
                         dev_name);
                exit(EXIT_FAILURE);
            }
            break;

		// Streaming mode has to be supported for both - memory mapping and user pointer methods to work
        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            if (!(cap.capabilities & V4L2_CAP_STREAMING))
            {
                fprintf(stderr, "%s does not support streaming i/o\n",
                         dev_name);
                exit(EXIT_FAILURE);
            }
            break;
    }


    /* Select video input, video standard and tune here. */

	// Set the entire structure to 0
    CLEAR(cropcap);

	// Seeting buffer type to buffer of a video capture stream
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#vidioc-cropcap
	// Proceed if querying information about the video cropping and scaling abilities succeeds
    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
    {
    	// Select the appropriate buffer
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        // https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-cropcap
        // Set the cropping rectangle to the default one
        crop.c = cropcap.defrect;

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#vidioc-g-crop
		// Open file to update the cropping parameters
        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
        {
            switch (errno)
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
    if (force_format)
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
        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                errno_exit("VIDIOC_S_FMT");

        /* Note VIDIOC_S_FMT may change width and height. */
    }
    else
    {
        printf("ASSUMING FORMAT\n");
        /* Preserve original settings as set by v4l2-ctl for example */
        if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                    errno_exit("VIDIOC_G_FMT");
    }

	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-pix-format
	// Looks like these parameters should be set automatically, however, as it says before, driver could be buggy
    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
            fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
            fmt.fmt.pix.sizeimage = min;

	// Call appropriate initialization function based on the command line argument passed
    switch (io)
    {
    	// Read functionality is requested
    	// Call the proper function with buffer size set to size of one image (all sizes are in bytes)
        case IO_METHOD_READ:
            init_read(fmt.fmt.pix.sizeimage);
            break;

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#mmap
		// Call the proper function to setup memory mapping background - that is specifically required
        case IO_METHOD_MMAP:
            init_mmap();
            break;

		// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#userp
		// Call the proper function to setup user pointer background - that is specifically required
		// Pass the size of the one frame as function parameter
        case IO_METHOD_USERPTR:
            init_userp(fmt.fmt.pix.sizeimage);
            break;
    }
}


static void close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");

        fd = -1;
}

static void open_device(void)
{

	// http://man7.org/linux/man-pages/man2/stat.2.html

        struct stat st;

		// Retrieve information about the file pointed to by dev_name
		// Also, store the same in st structure
		// Error handling
        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

		// https://www.gnu.org/software/libc/manual/html_node/Testing-File-Type.html
		// This macro returns non-zero if the file is a character special file (a device like a terminal)
        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no device\n", dev_name);
                exit(EXIT_FAILURE);
        }

		// Get access to camera
        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

		// Error handling
        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

static void usage(FILE *fp, int argc, char **argv)
{
        fprintf(fp,
                 "Usage: %s [options]\n\n"
                 "Version 1.3\n"
                 "Options:\n"
                 "-d | --device name   Video device name [%s]\n"
                 "-h | --help          Print this message\n"
                 "-m | --mmap          Use memory mapped buffers [default]\n"
                 "-r | --read          Use read() calls\n"
                 "-u | --userp         Use application allocated buffers\n"
                 "-o | --output        Outputs stream to stdout\n"
                 "-f | --format        Force format to 640x480 GREY\n"
                 "-c | --count         Number of frames to grab [%i]\n"
                 "",
                 argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmruofc:";

static const struct option
long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "read",   no_argument,       NULL, 'r' },
        { "userp",  no_argument,       NULL, 'u' },
        { "output", no_argument,       NULL, 'o' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
    if(argc > 1)
        dev_name = argv[1];
    else
        dev_name = "/dev/video0";

    for (;;)
    {
        int idx;
        int c;

        c = getopt_long(argc, argv,
                    short_options, long_options, &idx);

        if (-1 == c)
            break;

        switch (c)
        {
            case 0: /* getopt_long() flag */
                break;

            case 'd':
                dev_name = optarg;
                break;

            case 'h':
                usage(stdout, argc, argv);
                exit(EXIT_SUCCESS);

            case 'm':
                io = IO_METHOD_MMAP;
                break;

            case 'r':
                io = IO_METHOD_READ;
                break;

            case 'u':
                io = IO_METHOD_USERPTR;
                break;

            case 'o':
                out_buf++;
                break;

            case 'f':
                force_format++;
                break;

            case 'c':
                errno = 0;
                frame_count = strtol(optarg, NULL, 0);
                if (errno)
                        errno_exit(optarg);
                break;

            default:
                usage(stderr, argc, argv);
                exit(EXIT_FAILURE);
        }
    }

    open_device();
    init_device();
    start_capturing();
    mainloop();
    stop_capturing();
    uninit_device();
    close_device();
    fprintf(stderr, "\n");
    return 0;
}
