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

extern pthread_t Logger_Thread, Cam_Grey_Thread;

extern frame_p_buffer *frame_p;
static frame_p_buffer info_p;

static monitor_struct read_status;

static uint32_t frames = 1;

static log_struct outgoing_msg;

static int q_send_resp, q_recv_resp;

static mqd_t logger_queue, grey_queue, monitor_queue;

static struct timespec start_time_1, stop_time_1, start_time_2, stop_time_2, diff_time, curr_time;

static float difference_ms_1 = 0, frame_timings = 0, frame_rate_1 = 0, difference_ms_2 = 0, frame_rate_2 = 0, avg_fps = 0;

static uint8_t Logger_Q_Setup(void)
{
	// Queue setup
	struct mq_attr logger_queue_attr;
	logger_queue_attr.mq_maxmsg = queue_size;
	logger_queue_attr.mq_msgsize = sizeof(log_struct);

	logger_queue = mq_open(logger_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &logger_queue_attr);

	if(logger_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Logger Queue opening error for Cam_Monitor", Time_Stamp(Mode_ms));
		return 1;
	}

	outgoing_msg.source = Log_Monitor;

	return 0;
}

static uint8_t Grey_Q_Setup(void)
{
	// Queue setup
	struct mq_attr grey_queue_attr;
	grey_queue_attr.mq_maxmsg = queue_size;
	grey_queue_attr.mq_msgsize = sizeof(frame_p_buffer);

	grey_queue = mq_open(grey_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &grey_queue_attr);
	if(grey_queue == -1)
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Grey Queue opening Error");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		return 1;
	}
	return 0;
}

static uint8_t Monitor_Q_Setup(void)
{
	// Queue setup
	struct mq_attr monitor_queue_attr;
	monitor_queue_attr.mq_maxmsg = queue_size;
	monitor_queue_attr.mq_msgsize = sizeof(monitor_struct);

	monitor_queue = mq_open(monitor_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &monitor_queue_attr);
	if(monitor_queue == -1)
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Monitor Queue opening Error");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		return 1;
	}
	return 0;
}

static uint8_t read_frame(void)
{
	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buffer
	struct v4l2_buffer buf;

	uint32_t i;

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
			{
				return 1;
			}

			case EIO:
			/* Could ignore EIO, see spec. */

			/* fall through */

			default:
			{
				outgoing_msg.log_level = LOG_ERR;
				snprintf(outgoing_msg.message, Logging_Msg_Len, "VIDIOC_DQBUF Error");

				q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
				if(q_send_resp < 0)
				{
					syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
				}

				errno_exit("VIDIOC_DQBUF");
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

	q_send_resp = mq_send(grey_queue, &info_p, sizeof(frame_p_buffer),0);

	if(q_send_resp < 0)
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Grey Queue sending Error");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		pthread_exit(0);
	}

	// Process the image
	//    process_image((void *)buf.m.userptr, buf.bytesused);

	// Enqueue back the empty buffer to driver's incoming queue
	if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "VIDIOC_QBUF Error");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		errno_exit("VIDIOC_QBUF");
	}

	clock_gettime(CLOCK_REALTIME, &curr_time);
	if(curr_time.tv_nsec >= (1000000000 - 50000000))
	{
		curr_time.tv_sec += 1;
		curr_time.tv_nsec = 0;
	}
	
	else
	{
		curr_time.tv_nsec += 50000000;
	}

	q_recv_resp = mq_timedreceive(monitor_queue, &read_status, sizeof(monitor_struct), 0, &curr_time);

	if((q_recv_resp < 0) && (errno != ETIMEDOUT))
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Queue Receiving Failed");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		errno_exit("queue receive");
	}
	
	else
	{
		if(errno == ETIMEDOUT)
		{
			outgoing_msg.log_level = LOG_ERR;
			snprintf(outgoing_msg.message, Logging_Msg_Len, "Queue Receiving Timed Out");

			q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
			if(q_send_resp < 0)
			{
				syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
			}
		}

		else
		{
			if(read_status.Grey_status == Success_State)
			{
				read_status.Grey_status = Failed_State;
				outgoing_msg.log_level = LOG_INFO;
				snprintf(outgoing_msg.message, Logging_Msg_Len, "Greyscale Conversion Succeeded on Frame: %d", frames);

				q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
				if(q_send_resp < 0)
				{
					syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
				}
			}

			else
			{
				outgoing_msg.log_level = LOG_ERR;
				snprintf(outgoing_msg.message, Logging_Msg_Len, "Greyscale Conversion Failed");

				q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
				if(q_send_resp < 0)
				{
					syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
				}
			}
		}
	}

	return 0;
}

uint8_t device_warmup(void)
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
	if(-1 == r)
	{
		// If a signal made select() to return, then skip rest of the loop code, and start again
		if(EINTR == errno)
		{
			return 0;
		}

		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Select() Error");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		errno_exit("select");
	}

	// 0 return means that no file descriptor made the select() to close
	if(0 == r)
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Select() Timedout");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		errno_exit("select timeout");
	}

	// If select() indicated that device is ready for reading from it, then grab a frame
	if(read_frame() == 0)
	{
		outgoing_msg.log_level = LOG_INFO;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Device Wamup Successful");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}
	}

	else
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Device Wamup Failed");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}
	}
}

// Following function implements writer Thread
void *Cam_Monitor_Func(void *para_t)
{
	mq_unlink(grey_q_name);
	mq_unlink(monitor_q_name);

	read_status.Grey_status = Failed_State;
	read_status.RGB_Status = Failed_State;
	read_status.Compress_Status = Failed_State;

	if(Logger_Q_Setup() != 0)
	{
		syslog (LOG_ERR, "<%.6fms>Cam_Monitor Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	if(Grey_Q_Setup() != 0)
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Thread Setup Failed");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		pthread_exit(0);
	}

	if(Monitor_Q_Setup() != 0)
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Thread Setup Failed");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		pthread_exit(0);
	}

	outgoing_msg.log_level = LOG_INFO;
	snprintf(outgoing_msg.message, Logging_Msg_Len, "Thread Setup Completed on Core: %d", sched_getcpu());

	q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
	if(q_send_resp < 0)
	{
		syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
	}

	clock_gettime(CLOCK_REALTIME, &start_time_2);

	device_warmup();

	while(frames <= No_of_Frames)
	{

		clock_gettime(CLOCK_REALTIME, &start_time_1);

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
				continue;
			}

			outgoing_msg.log_level = LOG_ERR;
			snprintf(outgoing_msg.message, Logging_Msg_Len, "Select() Error");

			q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
			if(q_send_resp < 0)
			{
				syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
			}

			errno_exit("select");
		}

		// 0 return means that no file descriptor made the select() to close
		if(0 == r)
		{
			outgoing_msg.log_level = LOG_ERR;
			snprintf(outgoing_msg.message, Logging_Msg_Len, "Select() Timedout");

			q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
			if(q_send_resp < 0)
			{
				syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
			}

			errno_exit("select timeout");
		}

		// If select() indicated that device is ready for reading from it, then grab a frame
		if(read_frame() == 0)
		{
			// Indicate that one valid frame capturing has been completed
			frames += 1;

			clock_gettime(CLOCK_REALTIME, &stop_time_1);

			if(Max_Throughput == 1)
			{
				// Calculate Difference
				if(stop_time_1.tv_nsec >= start_time_1.tv_nsec)
				{
					difference_ms_1 = ((stop_time_1.tv_sec - start_time_1.tv_sec) * 1000) + (((float)(stop_time_1.tv_nsec - start_time_1.tv_nsec)) / 1000000);
				}

				else
				{
					difference_ms_1 = ((stop_time_1.tv_sec - start_time_1.tv_sec - 1) * 1000) + (((float)(1000000000 - (start_time_1.tv_nsec - stop_time_1.tv_nsec))) / 1000000);
				}

				frame_timings += ((float)1000 / difference_ms_1);
			}

			else
			{
				// Calculate Difference
				if(stop_time_1.tv_nsec >= start_time_1.tv_nsec)
				{
					diff_time.tv_sec = stop_time_1.tv_sec - start_time_1.tv_sec;
					diff_time.tv_nsec = stop_time_1.tv_nsec - start_time_1.tv_nsec;
				}

				else
				{
					diff_time.tv_sec = stop_time_1.tv_sec - start_time_1.tv_sec - 1;
					diff_time.tv_nsec = 1000000000 - (start_time_1.tv_nsec - stop_time_1.tv_nsec);
				}

				struct timespec rem_time;

				float ms1 = ((float)diff_time.tv_nsec / (float)1000000);
//				printf("\n Start1 sec: %d nsec: %d", start_time_1.tv_sec, start_time_1.tv_nsec);
//				printf("\n Stop1 sec: %d nsec: %d", stop_time_1.tv_sec, stop_time_1.tv_nsec);
//				printf("\n info - tvsec = %d, ms1 = %f, last thing = %f", diff_time.tv_sec, ms1, ((float)1000 / (float)Target_FPS));

				if((diff_time.tv_sec == 0) && (ms1 < ((float)1000 / (float)Target_FPS)))
				{
					diff_time.tv_nsec = (((1000 / Target_FPS) * 1000000) - diff_time.tv_nsec) * Margin_Factor;
					float ms2 = (diff_time.tv_nsec / (float)1000000);

					outgoing_msg.log_level = LOG_INFO;
					snprintf(outgoing_msg.message, Logging_Msg_Len, "Elapsed Time(ms): %f Sleeping For(ms): %f", ms1, ms2);

					q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
					if(q_send_resp < 0)
					{
						syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
					}
				}

				else
				{
					outgoing_msg.log_level = LOG_ERR;
					snprintf(outgoing_msg.message, Logging_Msg_Len, "Too much Latency", frame_rate_1);

					q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
					if(q_send_resp < 0)
					{
						syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
					}
				}

				do
				{
					clock_nanosleep(CLOCK_REALTIME, 0, &diff_time, &rem_time);
				}while(rem_time.tv_nsec > 0);
			}
		}
	}

	clock_gettime(CLOCK_REALTIME, &stop_time_2);
	
	// Calculate Difference
	if(stop_time_2.tv_nsec >= start_time_2.tv_nsec)
	{
		difference_ms_2 = ((stop_time_2.tv_sec - start_time_2.tv_sec) * 1000) + (((float)(stop_time_2.tv_nsec - start_time_2.tv_nsec)) / 1000000);
	}

	else
	{
		difference_ms_2 = ((stop_time_2.tv_sec - start_time_2.tv_sec - 1) * 1000) + (((float)(1000000000 - (start_time_2.tv_nsec - stop_time_2.tv_nsec))) / 1000000);
	}

	frame_rate_2 = ((float)1000 / (difference_ms_2 / No_of_Frames));

	outgoing_msg.log_level = LOG_INFO;
	snprintf(outgoing_msg.message, Logging_Msg_Len, "Frame Rate from Method 2 is: %.3f", frame_rate_2);

	q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
	if(q_send_resp < 0)
	{
		syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
	}

	if(Max_Throughput == 1)
	{
		frame_rate_1 = (frame_timings / ((float)No_of_Frames));

		outgoing_msg.log_level = LOG_INFO;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Frame Rate from Method 1 is: %.3f", frame_rate_1);

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}

		avg_fps = ((frame_rate_1 + frame_rate_2) / ((float)2));

		outgoing_msg.log_level = LOG_INFO;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Average FPS is: %.3f", avg_fps);

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);

		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Monitor", Time_Stamp(Mode_ms));
		}
	}

	usleep(100000);
	pthread_kill(Logger_Thread, SIGUSR1);
	usleep(100000);
	pthread_kill(Cam_Grey_Thread, SIGUSR1);
	usleep(100000);
	// Exit and log the termination event
	syslog(LOG_INFO, "<%.6fms>Cam_Monitor Exiting...", Time_Stamp(Mode_ms));

	mq_close(logger_queue);
	mq_close(grey_queue);
	mq_close(monitor_queue);
	mq_unlink(grey_q_name);
	mq_unlink(monitor_q_name);

	pthread_exit(0);
}
