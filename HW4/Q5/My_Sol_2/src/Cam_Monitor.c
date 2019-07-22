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

extern uint8_t Complete_Var;

extern pthread_t Logger_Thread, Cam_Grey_Thread, Cam_Brightness_Thread, Cam_Contrast_Thread;

extern uint8_t Terminate_Flag;
	
extern sem_t Monitor_Sem;

extern frame_p_buffer *frame_p;
static frame_p_buffer info_p;

static monitor_struct read_status;

static uint32_t frames = 1;

static log_struct outgoing_msg;

static int q_send_resp, q_recv_resp;

static mqd_t logger_queue, grey_queue, monitor_queue, brightness_queue, contrast_queue;

static struct timespec start_time_2, stop_time_2, diff_time, curr_time;

extern struct timespec start_time_1, stop_time_1;

static float difference_ms_2 = 0, frame_rate_2 = 0, avg_fps = 0;

static uint8_t warmup;

static uint8_t Grey_Q_Setup(void)
{
	// Queue setup
	struct mq_attr grey_queue_attr;
	grey_queue_attr.mq_maxmsg = queue_size;
	grey_queue_attr.mq_msgsize = sizeof(frame_p_buffer);

	grey_queue = mq_open(grey_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &grey_queue_attr);
	if(grey_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Grey Queue opening Error", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

static uint8_t Brightness_Q_Setup(void)
{
	// Queue setup
	struct mq_attr brightness_queue_attr;
	brightness_queue_attr.mq_maxmsg = queue_size;
	brightness_queue_attr.mq_msgsize = sizeof(frame_p_buffer);

	brightness_queue = mq_open(bright_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &brightness_queue_attr);
	if(brightness_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Brightness Queue opening Error", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

static uint8_t Contrast_Q_Setup(void)
{
	// Queue setup
	struct mq_attr contrast_queue_attr;
	contrast_queue_attr.mq_maxmsg = queue_size;
	contrast_queue_attr.mq_msgsize = sizeof(frame_p_buffer);

	contrast_queue = mq_open(contrast_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &contrast_queue_attr);
	if(contrast_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Contrast Queue opening Error", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

uint8_t read_frame(void)
{
	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buffer
	struct v4l2_buffer buf;

	uint32_t i;

	// Set the entire structure to 0
	CLEAR(buf);

	// Set proper type
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Indicate that the method being used is user pointer
	buf.memory = V4L2_MEMORY_USERPTR;

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

	// Ensure that the buffer received is a valid one
	for (i = 0; i < n_buffers; ++i)
	{
		if((buf.m.userptr == (unsigned long)frame_p[i].start) && (buf.length == frame_p[i].length))
		{
			break;
		}
	}

	assert(i < n_buffers);

	info_p.start = (void *) buf.m.userptr;
	info_p.length = buf.bytesused;

	if(warmup == 0)
	{

		q_send_resp = mq_send(grey_queue, &info_p, sizeof(frame_p_buffer),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Grey Queue sending Error", Time_Stamp(Mode_ms));
			pthread_exit(0);
		}

		q_send_resp = mq_send(brightness_queue, &info_p, sizeof(frame_p_buffer),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Brightness Queue sending Error", Time_Stamp(Mode_ms));
			pthread_exit(0);
		}

		q_send_resp = mq_send(contrast_queue, &info_p, sizeof(frame_p_buffer),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Contrast Queue sending Error", Time_Stamp(Mode_ms));
			pthread_exit(0);
		}

	}

	// Enqueue back the empty buffer to driver's incoming queue
	if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! VIDIOC_QBUF Error", Time_Stamp(Mode_ms));

		errno_exit("VIDIOC_QBUF");
	}

	return 0;
}

void device_warmup(void)
{
	warmup = 1; 

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
	if(-1 == r)
	{
		// If a signal made select() to return, then skip rest of the loop code, and start again
		if(EINTR == errno)
		{
			return 0;
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
	}

	warmup = 0;
}

// Following function implements writer Thread
void *Cam_Monitor_Func(void *para_t)
{

	if(Grey_Q_Setup() != 0) 
	{
		syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	if(Brightness_Q_Setup() != 0)
	{
		syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	if(Contrast_Q_Setup() != 0)
	{
		syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	syslog (LOG_INFO, "<%.6fms>!!Cam_Monitor!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	warmup = 0;

//	device_warmup();

	struct timespec diff_t, rem_t;
	int resp;

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

		// Set timeout of communication with Camera to 2 seconds
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

//		syslog (LOG_INFO, "<%.6fms>!!Cam_Monitor!! Compare Stamps", Time_Stamp(Mode_ms));

//		if((Complete_Var & Process_Complete_Mask) != 0)
//		{
//			Complete_Var &= ~(Process_Complete_Mask);

			clock_gettime(CLOCK_REALTIME, &start_time_1);

			// If select() indicated that device is ready for reading from it, then grab a frame
			if(read_frame() != 0)
			{
				syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Read_Frame() Error", Time_Stamp(Mode_ms));
			}
//		}

/*		diff_t.tv_sec = 0;
		diff_t.tv_nsec = (0.25 * Deadline_ms * ms_to_ns);

		do
		{
			resp = clock_nanosleep(CLOCK_REALTIME, 0, &diff_t, &rem_t);
			diff_t.tv_nsec = rem_t.tv_nsec;
		}while(resp != 0);*/

	}

	mq_close(grey_queue);
	mq_close(brightness_queue);
	mq_close(contrast_queue);

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Exiting...", Time_Stamp(Mode_ms));

	pthread_exit(0);
}
