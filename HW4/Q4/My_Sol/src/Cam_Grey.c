// Cam_Sharpen.c

#include "main.h"

extern struct v4l2_format fmt;

extern pthread_t Cam_Grey_Thread;

extern uint32_t n_buffers;

static mqd_t logger_queue, grey_queue, monitor_queue;

static frame_p_buffer info_p;

static monitor_struct write_status;

static log_struct outgoing_msg;

static int q_send_resp, q_recv_resp;

static uint32_t framecnt = 0;
static uint8_t bigbuffer[Big_Buffer_Size];
//static unsigned char bigbuffer[(1280*960)];

static void signal_function(int value)
{
	if((value == SIGUSR1) || (value == SIGUSR2))
	{
		syslog(LOG_INFO, "<%.6fms>Cam_Grey Exiting...", Time_Stamp(Mode_ms));
		mq_close(logger_queue);
		mq_close(grey_queue);
		mq_close(monitor_queue);
		pthread_exit(0);
	}
}

static void sig_setup(void)
{
		// Configuring timer and signal action
		struct sigaction custom_signal_action;

		// Set all initial values to 0 in the structure
		memset(&custom_signal_action, 0, sizeof (custom_signal_action));

		// Set signal action handler to point to the address of the target function (to execute on receiving signal)
		custom_signal_action.sa_handler = &signal_function;

		// Setting the signal action to kick in the handler function for these 2 signals
		sigaction(SIGUSR1, &custom_signal_action, 0);
		sigaction(SIGUSR2, &custom_signal_action, 0);
}

static uint8_t Logger_Q_Setup(void)
{
	// Queue setup
	struct mq_attr logger_queue_attr;
	logger_queue_attr.mq_maxmsg = queue_size;
	logger_queue_attr.mq_msgsize = sizeof(log_struct);

	logger_queue = mq_open(logger_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &logger_queue_attr);
	if(logger_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Logger Queue opening error for Cam_Grey", Time_Stamp(Mode_ms));
		return 1;
	}

	outgoing_msg.source = Log_Grey;

	return 0;
}

static uint8_t Grey_Q_Setup(void)
{
	// Queue setup
	struct mq_attr grey_queue_attr;
	grey_queue_attr.mq_maxmsg = queue_size;
	grey_queue_attr.mq_msgsize = sizeof(frame_p_buffer);

	grey_queue = mq_open(grey_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &grey_queue_attr);
	if(grey_queue == -1)
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Grey Queue opening Error");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Grey", Time_Stamp(Mode_ms));
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

	monitor_queue = mq_open(monitor_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &monitor_queue_attr);
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

char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char pgm_dumpname[]="img/test00000000.pgm";

void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time)
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

	// Close file
	close(dumpfd);

	outgoing_msg.log_level = LOG_INFO;
	snprintf(outgoing_msg.message, Logging_Msg_Len, "File Created, Wrote: %d Bytes", total);

	q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
	if(q_send_resp < 0)
	{
		syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Grey", Time_Stamp(Mode_ms));
	}

	write_status.Grey_status = Success_State;

	q_send_resp = mq_send(monitor_queue, &write_status, sizeof(monitor_struct),0);
	if(q_send_resp < 0)
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Status Update Sending Error");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Grey", Time_Stamp(Mode_ms));
		}
	}
}

static void process_image(const void *p, int size)
{
	int i, newi, newsize = 0;
	struct timespec frame_time;
	int y_temp, y2_temp, u_temp, v_temp;
	unsigned char *pptr = (unsigned char *)p;

	// Absolute timestamp - store the current time in structure
	clock_gettime(CLOCK_REALTIME, &frame_time);

	outgoing_msg.log_level = LOG_INFO;
	snprintf(outgoing_msg.message, Logging_Msg_Len, "Image Processing Start on Frame: %d of Size: %d", framecnt, size);

	q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
	if(q_send_resp < 0)
	{
		syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Grey", Time_Stamp(Mode_ms));
	}

	// Handle the required image processing for YUYV capture
	if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
	{
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
	}

	// Error handling
	else
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Unknown Format");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Grey", Time_Stamp(Mode_ms));
		}

		write_status.Grey_status = Failed_State;

		q_send_resp = mq_send(monitor_queue, &write_status, sizeof(monitor_struct),0);
		if(q_send_resp < 0)
		{
			outgoing_msg.log_level = LOG_ERR;
			snprintf(outgoing_msg.message, Logging_Msg_Len, "Status Update Sending Error");

			q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
			if(q_send_resp < 0)
			{
				syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Grey", Time_Stamp(Mode_ms));
			}
		}

	}

	// Increase framecount by 1
	framecnt += 1;

	fflush(stderr);
	//fprintf(stderr, ".");
	fflush(stdout);
}

void *Cam_Grey_Func(void *para_t)
{
	if(Logger_Q_Setup() != 0)
	{
		syslog (LOG_ERR, "<%.6fms>Cam_Grey Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	if(Grey_Q_Setup() != 0)
	{
		outgoing_msg.log_level = LOG_ERR;
		snprintf(outgoing_msg.message, Logging_Msg_Len, "Thread Setup Failed");

		q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Grey", Time_Stamp(Mode_ms));
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

    	sig_setup();

	outgoing_msg.log_level = LOG_INFO;
	snprintf(outgoing_msg.message, Logging_Msg_Len, "Thread Setup Completed on Core: %d", sched_getcpu());

	q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
	if(q_send_resp < 0)
	{
		syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Grey", Time_Stamp(Mode_ms));
	}

	while(1)
	{
		q_recv_resp = mq_receive(grey_queue, &info_p, sizeof(frame_p_buffer), 0);

		if(q_recv_resp < 0)
		{
			outgoing_msg.log_level = LOG_ERR;
			snprintf(outgoing_msg.message, Logging_Msg_Len, "Queue Receiving Failed");

			q_send_resp = mq_send(logger_queue, &outgoing_msg, sizeof(log_struct),0);
			if(q_send_resp < 0)
			{
				syslog(LOG_ERR, "<%.6fms>Logger Queue sending error for Cam_Grey", Time_Stamp(Mode_ms));
			}

			pthread_kill(Cam_Grey_Thread, SIGUSR1);
		}

		process_image((void *)info_p.start, info_p.length);
	}
}
