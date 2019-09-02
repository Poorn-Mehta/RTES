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
extern uint8_t mode, res;

extern float Grey_Exec_Time[No_of_Frames], Bright_Exec_Time[No_of_Frames], Contrast_Exec_Time[No_of_Frames];

extern float Grey_Avg_FPS[5], Grey_WCET[5], Grey_Avg_ET[5], Bright_Avg_FPS[5], Bright_WCET[5], Bright_Avg_ET[5], Contrast_Avg_FPS[5], Contrast_WCET[5], Contrast_Avg_ET[5];

extern pthread_t Logger_Thread, Cam_Grey_Thread, Cam_Brightness_Thread, Cam_Contrast_Thread;

extern frame_p_buffer *frame_p;
static frame_p_buffer info_p;

static monitor_struct read_status;

static uint32_t frames = 1;

static log_struct outgoing_msg;

static int q_send_resp, q_recv_resp;

static mqd_t logger_queue, grey_queue, monitor_queue, brightness_queue, contrast_queue;

static struct timespec start_time_1, stop_time_1, start_time_2, stop_time_2, diff_time, curr_time;

static float difference_ms_1 = 0, frame_timings = 0, frame_rate_1 = 0, difference_ms_2 = 0, frame_rate_2 = 0, avg_fps = 0;

static float Grey_Exec_Sum, Bright_Exec_Sum, Contrast_Exec_Sum;

static uint32_t framecnt;

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

/*		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Contrast Queue opening Error");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Contrast", Time_Stamp(Mode_ms));
		}*/

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

		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Monitor Queue opening Error", Time_Stamp(Mode_ms));

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

	switch(mode)
	{
		case 0:
		{
			q_send_resp = mq_send(grey_queue, &info_p, sizeof(frame_p_buffer),0);
			if(q_send_resp < 0)
			{
				syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Grey Queue sending Error", Time_Stamp(Mode_ms));
				pthread_exit(0);
			}
			break;
		}

		case 1:
		{
			q_send_resp = mq_send(brightness_queue, &info_p, sizeof(frame_p_buffer),0);
			if(q_send_resp < 0)
			{
				syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Brightness Queue sending Error", Time_Stamp(Mode_ms));
				pthread_exit(0);
			}
			break;
		}

		case 2:
		{
			q_send_resp = mq_send(contrast_queue, &info_p, sizeof(frame_p_buffer),0);
			if(q_send_resp < 0)
			{
				syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Contrast Queue sending Error", Time_Stamp(Mode_ms));
				pthread_exit(0);
			}
			break;
		}

		default:
		{
			break;
		}
	}

	// Process the image
	//    process_image((void *)buf.m.userptr, buf.bytesused);

	// Enqueue back the empty buffer to driver's incoming queue
	if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! VIDIOC_QBUF Error", Time_Stamp(Mode_ms));

		errno_exit("VIDIOC_QBUF");
	}

/*	clock_gettime(CLOCK_REALTIME, &curr_time);
	if(curr_time.tv_nsec >= (1000000000 - 500000000))
	{
		curr_time.tv_sec += 1;
		curr_time.tv_nsec = curr_time.tv_nsec - 500000000;
	}
	
	else
	{
		curr_time.tv_nsec += 500000000;
	}

	q_recv_resp = mq_timedreceive(monitor_queue, &read_status, sizeof(monitor_struct), 0, &curr_time);

	if((q_recv_resp < 0) && (errno != ETIMEDOUT))
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Contrast Queue receiving Error", Time_Stamp(Mode_ms));

		errno_exit("queue receive");
	}*/

	q_recv_resp = mq_receive(monitor_queue, &read_status, sizeof(monitor_struct), 0);

	if(q_recv_resp < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Contrast Queue receiving Error", Time_Stamp(Mode_ms));

		errno_exit("queue receive");
	}
	
	else
	{
/*		if(errno == ETIMEDOUT)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Contrast Queue receiving timeout Error", Time_Stamp(Mode_ms));
		}

		else 
		{*/
			switch(mode)
			{
				case 0:
				{
					if(read_status.Grey_status == Success_State)
					{
						read_status.Grey_status = Failed_State;
						syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Greyscale Conversion Succeeded on Frame: %d", Time_Stamp(Mode_ms), frames);
					}

					else
					{
						syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Greyscale Conversion Failed", Time_Stamp(Mode_ms));
					}
					break;
				}

				case 1:
				{
					if(read_status.Bright_status == Success_State)
					{
						read_status.Bright_status = Failed_State;
						syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Brightness Conversion Succeeded on Frame: %d", Time_Stamp(Mode_ms), frames);
					}

					else
					{
						syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Brightness Conversion Failed", Time_Stamp(Mode_ms));
					}
					break;
				}

				case 2:
				{
					if(read_status.Contrast_status == Success_State)
					{
						read_status.Contrast_status = Failed_State;
						syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Contrast Conversion Succeeded on Frame: %d", Time_Stamp(Mode_ms), frames);
					}

					else
					{
						syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Contrast Conversion Failed", Time_Stamp(Mode_ms));
					}
					break;
				}

				default:
				{
					break;
				}
			}
		//}
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
		syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Device Wamup Successful", Time_Stamp(Mode_ms));
	}

	else
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Device Wamup Failed", Time_Stamp(Mode_ms));
	}
}

// Following function implements writer Thread
void *Cam_Monitor_Func(void *para_t)
{
	read_status.Grey_status = Failed_State;
	read_status.Bright_status = Failed_State;
	read_status.Contrast_status = Failed_State;

	switch(mode)
	{
		case 0:
		{
			if(Grey_Q_Setup() != 0) 
			{
				syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Thread Setup Failed", Time_Stamp(Mode_ms));

				pthread_exit(0);
			}
			break;
		}

		case 1:
		{
			if(Brightness_Q_Setup() != 0)
			{
				syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Thread Setup Failed", Time_Stamp(Mode_ms));

				pthread_exit(0);
			}
			break;
		}

		case 2:
		{
			if(Contrast_Q_Setup() != 0)
			{
				syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Thread Setup Failed", Time_Stamp(Mode_ms));

				pthread_exit(0);
			}
			break;
		}

		default:
		{
			break;
		}
	}

	if(Monitor_Q_Setup() != 0)
	{
		syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Thread Setup Failed", Time_Stamp(Mode_ms));

		pthread_exit(0);
	}

	syslog (LOG_INFO, "<%.6fms>!!Cam_Monitor!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	clock_gettime(CLOCK_REALTIME, &start_time_2);

	frames = 0;

	frame_timings = 0;

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

			syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Select() Error", Time_Stamp(Mode_ms));

			errno_exit("select");
		}

		// 0 return means that no file descriptor made the select() to close
		if(0 == r)
		{
			syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Select() Timedout", Time_Stamp(Mode_ms));

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

				if((diff_time.tv_sec == 0) && (ms1 < ((float)1000 / (float)Target_FPS)))
				{
					diff_time.tv_nsec = (((1000 / Target_FPS) * 1000000) - diff_time.tv_nsec) * Margin_Factor;
					float ms2 = (diff_time.tv_nsec / (float)1000000);

					syslog (LOG_INFO, "<%.6fms>!!Cam_Monitor!! Elapsed Time(ms): %f Sleeping For(ms): %f", Time_Stamp(Mode_ms), ms1, ms2);
				}

				else
				{
					syslog (LOG_ERR, "<%.6fms>!!Cam_Monitor!! Too much Latency", Time_Stamp(Mode_ms));
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

	syslog (LOG_INFO, "<%.6fms>!!Cam_Monitor!! Frame Rate from Method 2 is: %.3f", Time_Stamp(Mode_ms), frame_rate_2);

	if(Max_Throughput == 1)
	{
		frame_rate_1 = (frame_timings / ((float)No_of_Frames));

		syslog (LOG_INFO, "<%.6fms>!!Cam_Monitor!! Frame Rate from Method 1 is: %.3f", Time_Stamp(Mode_ms), frame_rate_1);

		avg_fps = ((frame_rate_1 + frame_rate_2) / ((float)2));

		syslog (LOG_INFO, "<%.6fms>!!Cam_Monitor!! *****Average FPS for Resolution: %d*%d in Mode: %d is: %.3f*****", Time_Stamp(Mode_ms), HRES, VRES, mode, avg_fps);

		switch(mode)
		{
			case 0:
			{
				Grey_Avg_FPS[res] = avg_fps;
				break;
			}

			case 1:
			{
				Bright_Avg_FPS[res] = avg_fps;
				break;
			}

			case 2:
			{
				Contrast_Avg_FPS[res] = avg_fps;
				break;
			}

			default:
			{
				break;
			}
		}
	}
	
	switch(mode)
	{
		case 0:
		{
			Grey_WCET[res] = 0;
			Grey_Exec_Sum = 0;

			for(framecnt = 0; framecnt < No_of_Frames; framecnt ++)
			{
				Grey_Exec_Sum += Grey_Exec_Time[framecnt];
				if(Grey_Exec_Time[framecnt] > Grey_WCET[res])
				{
					Grey_WCET[res] = Grey_Exec_Time[framecnt];
				}
			}

			Grey_Avg_ET[res] = (Grey_Exec_Sum / ((float)No_of_Frames));

			break;
		}

		case 1:
		{
			Bright_WCET[res] = 0;
			Bright_Exec_Sum = 0;

			for(framecnt = 0; framecnt < No_of_Frames; framecnt ++)
			{
				Bright_Exec_Sum += Bright_Exec_Time[framecnt];
				if(Bright_Exec_Time[framecnt] > Bright_WCET[res])
				{
					Bright_WCET[res] = Bright_Exec_Time[framecnt];
				}
			}

			Bright_Avg_ET[res] = (Bright_Exec_Sum / ((float)No_of_Frames));

			break;
		}

		case 2:
		{
			Contrast_WCET[res] = 0;
			Contrast_Exec_Sum = 0;

			for(framecnt = 0; framecnt < No_of_Frames; framecnt ++)
			{
				Contrast_Exec_Sum += Contrast_Exec_Time[framecnt];
				if(Contrast_Exec_Time[framecnt] > Contrast_WCET[res])
				{
					Contrast_WCET[res] = Contrast_Exec_Time[framecnt];
				}
			}

			Contrast_Avg_ET[res] = (Contrast_Exec_Sum / ((float)No_of_Frames));

			break;
		}

		default:
		{
			break;
		}
	}
	

	usleep(100000);

	switch(mode)
	{
		case 0:
		{
			pthread_kill(Cam_Grey_Thread, SIGUSR1);
			mq_close(grey_queue);
			break;
		}

		case 1:
		{
			pthread_kill(Cam_Brightness_Thread, SIGUSR1);
			mq_close(brightness_queue);
			break;
		}

		case 2:
		{
			pthread_kill(Cam_Contrast_Thread, SIGUSR1);
			mq_close(contrast_queue);
			break;
		}

		default:
		{
			pthread_kill(Cam_Grey_Thread, SIGUSR1);
			mq_close(grey_queue);
			break;
		}
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Exiting...", Time_Stamp(Mode_ms));

//	mq_close(logger_queue);
	
	
	mq_close(monitor_queue);

	pthread_exit(0);
}
