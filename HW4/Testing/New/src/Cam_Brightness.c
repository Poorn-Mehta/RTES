#include "main.h"

struct v4l2_format fmt;

extern uint8_t Terminate_Flag, Operating_Mode;
extern uint8_t Complete_Var; 
extern sem_t Brightness_Sem;
extern pthread_t Cam_Brightness_Thread;
extern uint32_t n_buffers, Deadline_ms;
extern frame_p_buffer shared_struct;
extern uint8_t data_buffer[No_of_Buffers][Big_Buffer_Size];
extern strct_analyze Analysis;
extern struct utsname sys_info;

static frame_p_buffer info_p;
static uint32_t framecnt = 0, sock_frame = 0, brgt_index;
static uint8_t buf_index;
static store_struct file_out;
static mqd_t storage_queue, socket_queue;
static int q_send_resp;
static char ppm_header[]="P6\n#9999999999 sec 9999999999 msec \n#SYS_INFO: poorn-desktop aarch64\n"HRES_STR" "VRES_STR"\n255\n";
static char ppm_dumpname[]="img/r0/rgbc00000000.ppm";
static float Brightness_Stamp_1, Brightness_Stamp_2;
static float Brightness_Start_Stamp, Brightness_End_Stamp, Deadline_Stamp_1, Deadline_Stamp_2;
static char tmp_str[200];

static uint8_t Storage_Q_Setup(void)
{
    // Queue setup
	struct mq_attr storage_queue_attr;
	storage_queue_attr.mq_maxmsg = queue_size;
	storage_queue_attr.mq_msgsize = sizeof(store_struct);

	storage_queue = mq_open(storage_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &storage_queue_attr);
	if(storage_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Storage Queue opening error for Cam_Brightness", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

static uint8_t Socket_Q_Setup(void)
{
	// Queue setup
	struct mq_attr socket_queue_attr;
	socket_queue_attr.mq_maxmsg = queue_size;
	socket_queue_attr.mq_msgsize = sizeof(store_struct);

	socket_queue = mq_open(socket_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &socket_queue_attr);
	if(socket_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Socket Queue opening error for Cam_Brightness", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

static void dump_ppm(uint32_t size, uint32_t tag, uint32_t sock_tag, struct timespec *time)
{
//    int written, total, dumpfd;

	// Store timestamp and resolution in the header
	snprintf(&ppm_header[4], 11, "%010d", (int)time->tv_sec);
	strncat(&ppm_header[14], " sec ", 5);
	snprintf(&ppm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000)); 
//	strncat(&ppm_header[29], " msec \n"HRES_STR" "VRES_STR"\n255\n", 19);
	snprintf(&tmp_str[0], 200, " msec \n#SYS_INFO: %s %s\n"HRES_STR" "VRES_STR"\n255\n", sys_info.nodename, sys_info.machine);
	strncat(&ppm_header[29], tmp_str, 160);

	strncpy(ppm_dumpname, "img/r2/rgbc00000000.ppm", sizeof(ppm_dumpname));
	
/*	if((tag % 100) != 0)
	{
		strncpy(ppm_dumpname, "img/r2/rgbc00000000.ppm", sizeof(ppm_dumpname));
	}

	else
	{
		strncpy(ppm_dumpname, "img/an/rgbc00000000.ppm", sizeof(ppm_dumpname));
	}*/

	strncpy(&file_out.header[0], ppm_header, sizeof(ppm_header));
	file_out.headersize = sizeof(ppm_header);
	file_out.filesize = size;
	file_out.dataptr = &data_buffer[buf_index][0];
	file_out.total_frames = No_of_Frames;

	// Set name in a way that it reflects the frame number
	snprintf(&ppm_dumpname[11], 9, "%08d", tag);
	// Append .ppm at the last
	strncat(&ppm_dumpname[19], ".ppm", 5);
	strncpy(&file_out.filename[0], ppm_dumpname, sizeof(ppm_dumpname));

	q_send_resp = mq_send(storage_queue, (const char *)&file_out, sizeof(store_struct),0);
	if(q_send_resp < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Storage Queue sending Error", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	if((Operating_Mode & Mode_Socket_Mask) == Mode_Socket_ON_Val)
	{
		// Set name in a way that it reflects the frame number
		snprintf(&ppm_dumpname[11], 9, "%08d", sock_tag);
		// Append .ppm at the last
		strncat(&ppm_dumpname[19], ".ppm", 5);
		strncpy(&file_out.filename[0], ppm_dumpname, sizeof(ppm_dumpname));

		q_send_resp = mq_send(socket_queue, (const char *)&file_out, sizeof(store_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Socket Queue sending Error", Time_Stamp(Mode_ms));
	//		pthread_exit(0); //Non critical failure
		}
	}

	syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! File Sent to Storage Thread", Time_Stamp(Mode_ms));

	Complete_Var |= Brightness_Complete_Mask;

	buf_index += 1;	
	if(buf_index >= No_of_Buffers)
	{
		buf_index = 0;
	}
    
}

static void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b)
{
	int r1, g1, b1;

	// replaces floating point coefficients
	int c = y-16, d = u - 128, e = v - 128;       

	// Conversion that avoids floating point
	r1 = ((298 * c)           + (409 * e) + 128) >> 8;
	g1 = ((298 * c) - (100 * d) - (208 * e) + 128) >> 8;
	b1 = ((298 * c) + (516 * d)           + 128) >> 8;

	// Computed values may need clipping.

	*r = (r1 > 255) ? 255 : (r1 < 0 ? 0 : (uint8_t)r1);
	*g = (g1 > 255) ? 255 : (g1 < 0 ? 0 : (uint8_t)g1);
	*b = (b1 > 255) ? 255 : (b1 < 0 ? 0 : (uint8_t)b1);
}

static void process_image(const void *p, int size)
{
	int i, newi;
	struct timespec frame_time;
	unsigned char *pptr = (unsigned char *)p;
	int y_temp, y2_temp, u_temp, v_temp;

	// Absolute timestamp - store the current time in structure
	clock_gettime(CLOCK_REALTIME, &frame_time);

	if(sock_frame > 0)
	{
		Deadline_Stamp_2 = Time_Stamp(Mode_ms);

		if((Deadline_Stamp_2 - Deadline_Stamp_1) > ((float)Deadline_ms * Deadline_Overhead_Factor))
		{
			Analysis.Missed_Deadlines += 1;
			syslog (LOG_INFO, "!!WEIRD!! Diff: %.3f", Deadline_Stamp_2 - Deadline_Stamp_1);
		}

		Deadline_Stamp_1 = Time_Stamp(Mode_ms);
	}

	else
	{
		Deadline_Stamp_1 = Time_Stamp(Mode_ms);
	}

	syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! Image Processing Start on Frame: %d of Size: %d", Time_Stamp(Mode_ms), framecnt, size);

/*	if((framecnt % 100) != 0)
	{
		syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! Image Processing Start on Frame: %d of Size: %d", Time_Stamp(Mode_ms), framecnt, size);
	}

	else
	{
		syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! GREP_THIS Image Processing Start on Frame: %d of Size: %d", Time_Stamp(Mode_ms), framecnt, size);
	}*/

	// Handle the required image processing for YUYV capture
	if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
	{

		for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
		{
			y_temp=(int)pptr[i]; u_temp=(int)pptr[i+1]; y2_temp=(int)pptr[i+2]; v_temp=(int)pptr[i+3];

			yuv2rgb(y_temp, u_temp, v_temp, &data_buffer[buf_index][newi], &data_buffer[buf_index][newi+1], &data_buffer[buf_index][newi+2]);
			yuv2rgb(y2_temp, u_temp, v_temp, &data_buffer[buf_index][newi+3], &data_buffer[buf_index][newi+4], &data_buffer[buf_index][newi+5]);
		}

		dump_ppm(((size*6)/4), framecnt, sock_frame, &frame_time);
	}

	// Error handling
	else
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Brightness!! Unknown Format", Time_Stamp(Mode_ms));
	}

	// Increase framecount by 1
	framecnt += 1;
	sock_frame += 1;

	if(framecnt >= Wrap_around_Frames)
	{
		framecnt = 0;
	}

	fflush(stderr);
	//fprintf(stderr, ".");
	fflush(stdout);
}

void *Cam_Brightness_Func(void *para_t)
{
	framecnt = 0;
	buf_index = 0;
	sock_frame = 0;
	brgt_index = 0;

	if(Storage_Q_Setup() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Storage!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	if(Socket_Q_Setup() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Storage!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	while(1)
	{
		sem_wait(&Brightness_Sem);

		if(brgt_index == 0)
		{
			Brightness_Start_Stamp = Time_Stamp(Mode_ms);
		}

		Brightness_Stamp_1 = Time_Stamp(Mode_ms);

		if(Terminate_Flag != 0)
		{
			break;
		}

		info_p.start = shared_struct.start;
		info_p.length = shared_struct.length;

		if((Complete_Var & Brightness_Complete_Mask) == 0)
		{
			process_image((void *)info_p.start, info_p.length);
		}

		Brightness_Stamp_2 = Time_Stamp(Mode_ms);

		Analysis.Exec_Analysis.Brightness_Exec[brgt_index] = Brightness_Stamp_2 - Brightness_Stamp_1;

		brgt_index += 1;
	}

	Brightness_End_Stamp = Time_Stamp(Mode_ms);

	Analysis.Jitter_Analysis.Overall_Jitter[Brightness_TID] = Brightness_End_Stamp - (Brightness_Start_Stamp + (No_of_Frames * Deadline_ms));

	mq_close(storage_queue);

	syslog(LOG_INFO, "<%.6fms>!!Cam_Brightness!! Exiting...", Time_Stamp(Mode_ms));

	pthread_exit(0);
}





