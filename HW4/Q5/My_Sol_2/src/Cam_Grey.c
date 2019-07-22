#include "main.h"

struct v4l2_format fmt;

extern uint8_t Terminate_Flag;

extern uint8_t Complete_Var; 

extern float Grey_Exec_Rem[];

extern uint32_t Grey_No_Frame[5];
	
extern sem_t Grey_Sem;

extern uint32_t HRES, VRES;
extern uint8_t res;

extern pthread_t Cam_Grey_Thread;

extern uint32_t n_buffers;

static mqd_t grey_queue, monitor_queue;

static frame_p_buffer info_p;

static monitor_struct write_status;

static log_struct outgoing_msg;

static int q_send_resp, q_recv_resp;

static uint32_t framecnt = 0, old_framcnt = 0;
static uint8_t bigbuffer[Big_Buffer_Size];

static float us_tstamp1, us_tstamp2; 
static uint32_t i;

static uint8_t Grey_Q_Setup(void)
{
	// Queue setup
	struct mq_attr grey_queue_attr;
	grey_queue_attr.mq_maxmsg = queue_size;
	grey_queue_attr.mq_msgsize = sizeof(frame_p_buffer);

	grey_queue = mq_open(grey_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &grey_queue_attr);
	if(grey_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Grey!! Grey Queue opening Error", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

static char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
static char pgm_dumpname[]="img/r0/grey00000000.pgm";

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
			strncpy(pgm_dumpname, "img/r0/grey00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		case 1:
		{
			strncat(&pgm_header[29], " msec \n800 600\n255\n", 19);
			strncpy(pgm_dumpname, "img/r1/grey00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		case 2:
		{
			strncat(&pgm_header[29], " msec \n640 480\n255\n", 19);
			strncpy(pgm_dumpname, "img/r2/grey00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		case 3:
		{
			strncat(&pgm_header[29], " msec \n320 240\n255\n", 19);
			strncpy(pgm_dumpname, "img/r3/grey00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		case 4:
		{
			strncat(&pgm_header[29], " msec \n160 120\n255\n", 19);
			strncpy(pgm_dumpname, "img/r4/grey00000000.pgm", sizeof(pgm_dumpname));
			break;
		}

		default:
		{
			strncat(&pgm_header[29], " msec \n960 720\n255\n", 19);
			strncpy(pgm_dumpname, "img/r0/grey00000000.pgm", sizeof(pgm_dumpname));
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

	syslog(LOG_INFO, "<%.6fms>!!Cam_Grey!! File Created, Wrote: %d Bytes", Time_Stamp(Mode_ms), total);

	Complete_Var |= Grey_Complete_Mask;
}

static void process_image(const void *p, int size)
{
	int i, newi, newsize = 0, tmp;
	struct timespec frame_time;
	int y_temp, y2_temp, u_temp, v_temp;
	unsigned char *pptr = (unsigned char *)p;

	// Absolute timestamp - store the current time in structure
	clock_gettime(CLOCK_REALTIME, &frame_time);

	syslog(LOG_INFO, "<%.6fms>!!Cam_Grey!! Image Processing Start on Frame: %d of Size: %d", Time_Stamp(Mode_ms), framecnt, size);

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
		syslog(LOG_ERR, "<%.6fms>!!Cam_Grey!! Unknown Format", Time_Stamp(Mode_ms));

		write_status.Grey_status = Failed_State;

		q_send_resp = mq_send(monitor_queue, &write_status, sizeof(monitor_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Grey!! Status Update Sending Error", Time_Stamp(Mode_ms));
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
	framecnt = 0;
//	old_framcnt = 0;
	us_tstamp1 = 0;
	us_tstamp2 = 0;
	i = 0;
	Grey_No_Frame[res] = 0;

	if(Grey_Q_Setup() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Grey!! Thread Setup Failed", Time_Stamp(Mode_ms));

		pthread_exit(0);
	}

	syslog(LOG_INFO, "<%.6fms>!!Cam_Grey!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	static struct timespec curr_time;
	struct timespec stamp1, stamp2;
	float tmpstamp;

	while(1)
	{

		clock_gettime(CLOCK_REALTIME, &stamp1);	

//		us_tstamp1 = Time_Stamp(Mode_us);

		if(i > 0)
		{
//			Grey_Exec_Rem[i-1] = ((float)(Deadline_ms * ms_to_us) - (us_tstamp1 - us_tstamp2));
			if(stamp1.tv_nsec >= stamp2.tv_nsec)
			{
				Grey_Exec_Rem[i-1] = (float)((stamp1.tv_sec - stamp2.tv_sec) * s_to_us) + (float)((stamp1.tv_nsec - stamp2.tv_nsec) * ns_to_us);
			}
	
			else
			{
				Grey_Exec_Rem[i-1] = (float)((stamp1.tv_sec - stamp2.tv_sec - 1) * s_to_us) + (float)((s_to_ns - (stamp2.tv_nsec - stamp1.tv_nsec)) * ns_to_us);
			}

			Grey_Exec_Rem[i-1] = (float)(Deadline_ms * ms_to_us) - Grey_Exec_Rem[i-1];
		}

		sem_wait(&Grey_Sem);

		clock_gettime(CLOCK_REALTIME, &stamp2);	

//		us_tstamp2 = Time_Stamp(Mode_us);

		i += 1;

//		old_framcnt = framecnt;
		
		if(Terminate_Flag != 0)
		{
			break;
		}

		clock_gettime(CLOCK_REALTIME, &curr_time);

		q_recv_resp = mq_timedreceive(grey_queue, &info_p, sizeof(frame_p_buffer), 0, &curr_time);

		if((q_recv_resp < 0) && (errno != ETIMEDOUT))
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Grey!! Queue receiving Error", Time_Stamp(Mode_ms));
			errno_exit("queue receive");
		}

		else if(q_recv_resp == sizeof(frame_p_buffer))
		{
			if((Complete_Var & Grey_Complete_Mask) == 0)
			{
				process_image((void *)info_p.start, info_p.length);
			}

/*			do
			{
				clock_gettime(CLOCK_REALTIME, &curr_time);
				q_recv_resp = mq_timedreceive(grey_queue, &info_p, sizeof(frame_p_buffer), 0, &curr_time);
			} while(q_recv_resp == sizeof(frame_p_buffer));*/

		}

		else if(errno == ETIMEDOUT)
		{
			Grey_No_Frame[res] += 1;
		}
	}
	syslog(LOG_INFO, "<%.6fms>!!Cam_Grey!! Exiting...", Time_Stamp(Mode_ms));
	pthread_exit(0);
}
