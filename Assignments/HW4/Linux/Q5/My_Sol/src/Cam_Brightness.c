#include "main.h"

struct v4l2_format fmt;

extern uint32_t HRES, VRES;
extern uint8_t mode, res;

extern pthread_t Cam_Brightness_Thread;

extern float Bright_Exec_Time[No_of_Frames];

extern uint32_t n_buffers;

static mqd_t brightness_queue, monitor_queue;

static frame_p_buffer info_p;

static monitor_struct write_status;

static log_struct outgoing_msg;

static int q_send_resp, q_recv_resp;

static uint32_t framecnt = 0;
static uint8_t bigbuffer[Big_Buffer_Size];

static float us_tstamp1, us_tstamp2; 
static uint32_t framecnt_old;

static void signal_function(int value)
{
	if((value == SIGUSR1) || (value == SIGUSR2))
	{
		syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! Exiting...", Time_Stamp(Mode_ms));
		mq_close(brightness_queue);
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

static uint8_t Brightness_Q_Setup(void)
{
	// Queue setup
	struct mq_attr brightness_queue_attr;
	brightness_queue_attr.mq_maxmsg = queue_size;
	brightness_queue_attr.mq_msgsize = sizeof(frame_p_buffer);

	brightness_queue = mq_open(bright_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &brightness_queue_attr);
	if(brightness_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Brightness Queue opening Error", Time_Stamp(Mode_ms));

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
		syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Monitor Queue opening Error", Time_Stamp(Mode_ms));

		return 1;
	}
	return 0;
}

static char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
static char pgm_dumpname[]="img/r0/brgt00000000.pgm";

static void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time)
{
	int written, i, total, dumpfd;

	// Store timestamp and resolution in the header
	snprintf(&pgm_header[4], 11, "%010d", (int)time->tv_sec);
	strncat(&pgm_header[14], " sec ", 5);
	snprintf(&pgm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));
	switch(res)
	{
		case 0:
		{
			strncat(&pgm_header[29], " msec \n960 720\n255\n", 19);
			strncpy(pgm_dumpname, "img/r0/brgt00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		case 1:
		{
			strncat(&pgm_header[29], " msec \n800 600\n255\n", 19);
			strncpy(pgm_dumpname, "img/r1/brgt00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		case 2:
		{
			strncat(&pgm_header[29], " msec \n640 480\n255\n", 19);
			strncpy(pgm_dumpname, "img/r2/brgt00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		case 3:
		{
			strncat(&pgm_header[29], " msec \n320 240\n255\n", 19);
			strncpy(pgm_dumpname, "img/r3/brgt00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		case 4:
		{
			strncat(&pgm_header[29], " msec \n160 120\n255\n", 19);
			strncpy(pgm_dumpname, "img/r4/brgt00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		default:
		{
			strncat(&pgm_header[29], " msec \n960 720\n255\n", 19);
			strncpy(pgm_dumpname, "img/r0/brgt00000000.pgm", sizeof(pgm_dumpname));
			break;
		}
	}

	// Set name in a way that it reflects the frame number
	snprintf(&pgm_dumpname[11], 9, "%08d", tag);

	// Append .pgm at the last
	strncat(&pgm_dumpname[19], ".pgm", 5);

	// Create the file
	dumpfd = open(pgm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
	

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

	syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! File Created, Wrote: %d Bytes", Time_Stamp(Mode_ms), total);

	write_status.Bright_status = Success_State;

	q_send_resp = mq_send(monitor_queue, &write_status, sizeof(monitor_struct),0);
	if(q_send_resp < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Status Update Sending Error", Time_Stamp(Mode_ms));
	}
}

static void process_image(const void *p, int size)
{
	int i, newi, newsize = 0, tmp;
	struct timespec frame_time;
	int y_temp, y2_temp, u_temp, v_temp;
	unsigned char *pptr = (unsigned char *)p;

	// Absolute timestamp - store the current time in structure
	clock_gettime(CLOCK_REALTIME, &frame_time);

	syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! Image Processing Start on Frame: %d of Size: %d", Time_Stamp(Mode_ms), framecnt, size);

	// Handle the required image processing for YUYV capture
	if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
	{
		for(i=0, newi=0; i<size; i=i+4, newi=newi+2)
		{
			// Y1=first byte and Y2=third byte
			tmp = pptr[i] * Brightness_Factor;
			if(tmp > 255)
			{
				tmp = 255;
			}
			bigbuffer[newi]=tmp;
			tmp = pptr[i+2] * Brightness_Factor;
			if(tmp > 255)
			{
				tmp = 255;
			}
			bigbuffer[newi+1]=tmp;
		}

		dump_pgm(bigbuffer, (size/2), framecnt, &frame_time);
	}

	// Error handling
	else
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Unknown Format", Time_Stamp(Mode_ms));

		write_status.Bright_status = Failed_State;

		q_send_resp = mq_send(monitor_queue, &write_status, sizeof(monitor_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Status Update Sending Error", Time_Stamp(Mode_ms));
		}

	}

	// Increase framecount by 1
	framecnt += 1;

	fflush(stderr);
	//fprintf(stderr, ".");
	fflush(stdout);
}

void *Cam_Brightness_Func(void *para_t)
{
	framecnt = 0;

	if(Brightness_Q_Setup() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Thread Setup Failed", Time_Stamp(Mode_ms));

		pthread_exit(0);
	}

	if(Monitor_Q_Setup() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Thread Setup Failed", Time_Stamp(Mode_ms));

		pthread_exit(0);
	}

    	sig_setup();

	syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	while(1)
	{

		framecnt_old = framecnt;

		q_recv_resp = mq_receive(brightness_queue, &info_p, sizeof(frame_p_buffer), 0);

		us_tstamp1 = Time_Stamp(Mode_us);

		if(q_recv_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Queue Receiving Failed", Time_Stamp(Mode_ms));

			pthread_kill(Cam_Brightness_Thread, SIGUSR1);
		}

		process_image((void *)info_p.start, info_p.length);

		us_tstamp2 = Time_Stamp(Mode_us);

		if(framecnt_old < framecnt)
		{
			Bright_Exec_Time[framecnt-1] = (us_tstamp2 - us_tstamp1);
		}
	}
}





