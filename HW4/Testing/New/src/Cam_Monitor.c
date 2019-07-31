// Cam_Monitor.c

/*
*		File: Writer_Thread.c
*		Purpose: The source file containing implementation of writer thread for demonstrating timed Mutex Locks
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#include "main.h"

extern int fd;
extern uint32_t n_buffers;
extern uint8_t Max_Throughput;
extern uint32_t HRES, VRES;

extern uint8_t Terminate_Flag;

extern frame_p_buffer *frame_p;

//extern pthread_mutex_t Mutex_Locker;
extern frame_p_buffer shared_struct;

extern sem_t Monitor_Sem;

extern struct timespec start_time_1, stop_time_1;

static uint8_t warm = 1;

//mmap
uint8_t read_frame(void)
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
				syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! VIDIOC_DQBUF Error", Time_Stamp(Mode_ms));

				errno_exit("VIDIOC_DQBUF");
			
				break;
			}
		}
	}

	assert(buf.index < n_buffers);

	if(warm == 0)
	{
		shared_struct.start = (void *) frame_p[buf.index].start;
		shared_struct.length = buf.bytesused;
	}

	// Enqueue back the empty buffer to driver's incoming queue
	if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! VIDIOC_QBUF Error", Time_Stamp(Mode_ms));

		errno_exit("VIDIOC_QBUF");
	}

	return 0;
}


int device_warmup(void)
{
	// Declare set to use with FD_macros
	fd_set fds;

	// Structure to store timing values
	struct timeval tv;

	int r;

	warm = 1;

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
	if(-1 == r)
	{
		// If a signal made select() to return, then skip rest of the loop code, and start again
		if(EINTR == errno)
		{
			return 1;
		}

		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Select() Error", Time_Stamp(Mode_ms));

		errno_exit("select");
	}

	// 0 return means that no file descriptor made the select() to close
	if(0 == r)
	{

		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Select() Timedout", Time_Stamp(Mode_ms));

		errno_exit("select timeout");
	}

	// If select() indicated that device is ready for reading from it, then grab a frame
	if(read_frame() == 0)
	{
		syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Device Warmup Successful", Time_Stamp(Mode_ms));
	}

	else
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Device Warmup Failed", Time_Stamp(Mode_ms));
		return 1;
	}

	warm = 0;

	return 0;
}

// Following function implements writer Thread
void *Cam_Monitor_Func(void *para_t)
{

	syslog (LOG_INFO, "<%.6fms>!!Cam_Monitor!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	while(1)
	{

		sem_wait(&Monitor_Sem);
		
		if(Terminate_Flag != 0)
		{
			break;
		}

		// Declare set to use with FD_macros
		fd_set fds;

		// Structure to store timing values
		struct timeval tv;

		int r;

		// Clear the set
		FD_ZERO(&fds);

		// Store file descriptor of camera in the set
		FD_SET(fd, &fds);

		// Set timeout of communication with Camera to 25 ms

		tv.tv_sec = 0;
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
		if(-1 == r)
		{
			// If a signal made select() to return, then skip rest of the loop code, and start again
			if(EINTR == errno)
			{
				continue;
			}

			syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Select() Error", Time_Stamp(Mode_ms));

			errno_exit("select");
		}

		// 0 return means that no file descriptor made the select() to close
		if(0 == r)
		{
			continue;
		}

		// If select() indicated that device is ready for reading from it, then grab a frame
		if(read_frame() != 0)
		{
			syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Read_Frame() Error", Time_Stamp(Mode_ms));
		}
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Exiting...", Time_Stamp(Mode_ms));

	pthread_exit(0);
}
