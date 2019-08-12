/*
*		File: Cam_RGB.c
*		Purpose: The source file containing functions to convert the raw frame residing in
*			 memory mapped buffer, to a RGB frame, in real time.
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*
*		Some of the functions in this source file have been developed with reference to the
*		code developed by Professor Sam Siewert. The source code can be found in the below link.
*		http://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/computer-vision/simple-capture/
*
*/

#include "main.h"
#include "Aux_Func.h"
#include "Cam_RGB.h"

// Shared Variables
struct v4l2_format fmt;
uint8_t Terminate_Flag, Operating_Mode;
uint8_t Complete_Var; 
sem_t RGB_Sem;
uint32_t n_buffers, Deadline_ms;
frame_p_buffer shared_struct;
uint8_t data_buffer[No_of_Buffers][Big_Buffer_Size];
strct_analyze Analysis;
struct utsname sys_info;
float RGB_Start_Stamp, RGB_Stamp_1;

// Local Variables
static frame_p_buffer info_p;
static uint32_t framecnt = 0, sock_frame = 0, RGB_index;
static uint8_t buf_index, skip;
static store_struct file_out;
static mqd_t storage_queue, socket_queue;
static int q_send_resp;
static char ppm_header[]="P6\n#TIME_STAMP: 9999999999 sec 9999999999 msec \n#SYS_INFO: poorn-desktop aarch64\n"HRES_STR" "VRES_STR"\n255\n";
static char ppm_dumpname[]="img/r0/RGB01Hz00000000.ppm";
static float RGB_Stamp_2;
static float RGB_End_Stamp, Deadline_Stamp_1, Deadline_Stamp_2;

// Function to setup Storage Queue in Write Only & Nonblocking mode
// Parameter1: void
// Return: uint8_t result - 0: success
static uint8_t Storage_Q_Setup(void)
{
	struct mq_attr storage_queue_attr;
	storage_queue_attr.mq_flags = O_NONBLOCK;
	storage_queue_attr.mq_maxmsg = queue_size;
	storage_queue_attr.mq_msgsize = sizeof(store_struct);

	storage_queue = mq_open(storage_q_name, O_CREAT | O_WRONLY | O_NONBLOCK | O_CLOEXEC, 0666, &storage_queue_attr);
	if(storage_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Storage Queue opening error for Cam_RGB", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

// Function to setup Socket Queue in Write Only & Nonblocking mode
// Parameter1: void
// Return: uint8_t result - 0: success
static uint8_t Socket_Q_Setup(void)
{
	struct mq_attr socket_queue_attr;
	socket_queue_attr.mq_flags = O_NONBLOCK;
	socket_queue_attr.mq_maxmsg = queue_size;
	socket_queue_attr.mq_msgsize = sizeof(store_struct);

	socket_queue = mq_open(socket_q_name, O_CREAT | O_WRONLY | O_NONBLOCK | O_CLOEXEC, 0666, &socket_queue_attr);
	if(socket_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Socket Queue opening error for Cam_RGB", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

// This function was originally written by Professor Sam Siewert, as per mentioned at the top of this file
// Function to prepare converted image's header, filename etc
// and to share that with best effort threads using queue
// Parameter1: uint32_t size - converted image size
// Parameter2: uint32_t tag - image number 
// Parameter3: uint32_t sock_tag - image number for socket
// Parameter4: struct timespec *time - pointer to structure that has recorded the timestamp of when the frame was processed
// Return: void
static void dump_ppm(uint32_t size, uint32_t tag, uint32_t sock_tag, struct timespec *time)
{

	// Prepare the entire header
	if(snprintf(&ppm_header[0], PPM_Header_Max_Length, "P6\n#TIME_STAMP: %010d sec %010d msec \n#SYS_INFO: %s %s\n"HRES_STR" "VRES_STR"\n255\n", 
		(int)time->tv_sec, (int)((time->tv_nsec)/1000000), sys_info.nodename, sys_info.machine) < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Error in snprintf while writing header", Time_Stamp(Mode_ms));
	}

	// Prepare the file name based on the operating mode
	if((Operating_Mode & Mode_FPS_Mask) == Mode_10_FPS_Val)
	{
		strncpy(ppm_dumpname, "img/r2/RGB10Hz00000000.ppm", sizeof(ppm_dumpname));
	}

	else
	{
		strncpy(ppm_dumpname, "img/r2/RGB01Hz00000000.ppm", sizeof(ppm_dumpname));
	}

	// Copy all useful data about image in a structure
	strncpy(&file_out.header[0], ppm_header, sizeof(ppm_header));
	file_out.headersize = sizeof(ppm_header);
	file_out.filesize = size;
	file_out.dataptr = &data_buffer[buf_index][0];
	file_out.total_frames = No_of_Frames;

	// Set name in a way that it reflects the frame number
	if(snprintf(&ppm_dumpname[14], 9, "%08d", tag) < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Error in snprintf while writing filename", Time_Stamp(Mode_ms));
	}

	// Append .ppm at the last
	strncat(&ppm_dumpname[22], ".ppm", 5);
	strncpy(&file_out.filename[0], ppm_dumpname, sizeof(ppm_dumpname));

	// Send data to storage thread
	q_send_resp = mq_send(storage_queue, (const char *)&file_out, sizeof(store_struct),0);
	if(q_send_resp < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Storage Queue sending Error", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	// Send data to Socket thread
	if((Operating_Mode & Mode_Socket_Mask) == Mode_Socket_ON_Val)
	{
		// Set name in a way that it reflects the frame number
		if(snprintf(&ppm_dumpname[14], 9, "%08d", sock_tag) < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Error in snprintf while writing filename", Time_Stamp(Mode_ms));
		}

		// Append .ppm at the last
		strncat(&ppm_dumpname[22], ".ppm", 5);
		strncpy(&file_out.filename[0], ppm_dumpname, sizeof(ppm_dumpname));

		q_send_resp = mq_send(socket_queue, (const char *)&file_out, sizeof(store_struct),0);
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Socket Queue sending Error", Time_Stamp(Mode_ms));
	//		pthread_exit(0); //Non critical failure
		}
	}

	syslog(LOG_INFO, "<%.6fms>!!Cam_RGB!! File Sent to Storage Thread", Time_Stamp(Mode_ms));

	// Indicator for Scheduler that the frame processing in real time has been completed
	Complete_Var |= RGB_Complete_Mask;

	// Circular buffer indexing
	buf_index += 1;	
	if(buf_index >= No_of_Buffers)
	{
		buf_index = 0;
	}
    
}

// This function was originally written by Professor Sam Siewert, as per mentioned at the top of this file
// Function to convert YUYV raw frame data to a RGB frame
// Parameter1..3: YUV input values
// Parameter4..6: RGB output location pointers
// Return: void
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

// This function was originally written by Professor Sam Siewert, as per mentioned at the top of this file
// Function to process the raw frame pixel by pixel, and call other function to prepare file with appropriate parameters
// Parameter1: const void *p - pointer to memory mapped region where the actual raw frame data is stored
// Parameter2: int size - the length in bytes of the raw frame data array
// Return: void
static void process_image(const void *p, int size)
{
	// Local variables
	int i, newi;
	struct timespec frame_time;
	unsigned char *pptr = (unsigned char *)p;
	int y_temp, y2_temp, u_temp, v_temp;

	// Absolute timestamp - store the current time in structure
	if(clock_gettime(CLOCK_REALTIME, &frame_time) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Couldn't get time for frame_time", Time_Stamp(Mode_ms));
	}

	// Analysis
	if(sock_frame > 0)
	{
		Deadline_Stamp_2 = Time_Stamp(Mode_ms);

		// Deadline is only missed if it exceeds the predetermined number by more than 1%
		if((Deadline_Stamp_2 - Deadline_Stamp_1) > ((float)Deadline_ms * Deadline_Overhead_Factor))
		{
			Analysis.Missed_Deadlines += 1;
			syslog(LOG_INFO, "!!Cam_RGB!! Deadline Missed by Difference: %.3f", Deadline_Stamp_2 - Deadline_Stamp_1);
		}

		Deadline_Stamp_1 = Time_Stamp(Mode_ms);
	}

	else
	{
		Deadline_Stamp_1 = Time_Stamp(Mode_ms);
	}

	syslog(LOG_INFO, "<%.6fms>!!Cam_RGB!! Image Processing Start on Frame: %d of Size: %d", Time_Stamp(Mode_ms), framecnt, size);

	// Handle the required image processing for YUYV capture
	if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
	{
		// Directly storing the result in circular buffer
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
		syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Unknown Format", Time_Stamp(Mode_ms));
	}

	// For useless frames
	if(skip == 0)
	{
		// Increase framecount by 1
		framecnt += 1;
		sock_frame += 1;

		if(framecnt >= Wrap_around_Frames)
		{
			framecnt = 0;
		}
	}

	else
	{
		skip -= 1;
	}

	if(fflush(stderr) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Error while flushing stderr", Time_Stamp(Mode_ms));
	}

	if(fflush(stdout) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Error while flushing stdout", Time_Stamp(Mode_ms));
	}
}

// Function that implements Cam_RGB Thread
void *Cam_RGB_Func(void *para_t)
{
	// Setup
	framecnt = 0;
	buf_index = 0;
	sock_frame = 0;
	RGB_index = 0;
	skip = Useless_Frames;

	// Opening Queues
	if(Storage_Q_Setup() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Storage!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	if((Operating_Mode & Mode_Socket_Mask) == Mode_Socket_ON_Val)
	{
		if(Socket_Q_Setup() != 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Storage!! Thread Setup Failed", Time_Stamp(Mode_ms));
			pthread_exit(0);
		}
	}

	syslog(LOG_INFO, "<%.6fms>!!Cam_RGB!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	while(1)
	{
		// Wait for semaphore (from scheduler)
		if(sem_wait(&RGB_Sem) != 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Couldn't wait on RGB_Sem", Time_Stamp(Mode_ms));
			continue;
		}

		// For Rate Monotonic Analysis
		syslog(LOG_INFO, "<%.6fms>!!RMA!! Cam_RGB Launched (Iteration: %d)", Time_Stamp(Mode_ms), RGB_index);

		// Check if scheduler has raised this flag to terminate real time services
		if(Terminate_Flag != 0)
		{
			break;
		}

		// Copying the pointer locally
		info_p.start = shared_struct.start;
		info_p.length = shared_struct.length;

		// Processing only if Scheduler has cleared the bit mask
		if((Complete_Var & RGB_Complete_Mask) == 0)
		{
			process_image((void *)info_p.start, info_p.length);
		}

		RGB_Stamp_2 = Time_Stamp(Mode_ms);

		Analysis.Exec_Analysis.RGB_Exec[RGB_index] = RGB_Stamp_2 - RGB_Stamp_1;

		// For Rate Monotonic Analysis
		syslog(LOG_INFO, "<%.6fms>!!RMA!! Cam_RGB Completed (Iteration: %d)", Time_Stamp(Mode_ms), RGB_index);

		RGB_index += 1;
	}

	RGB_End_Stamp = Time_Stamp(Mode_ms);

	Analysis.Jitter_Analysis.Overall_Jitter[RGB_TID] = RGB_End_Stamp - (RGB_Start_Stamp + (No_of_Frames * Deadline_ms));

	// CLosing queues
	if(mq_close(storage_queue) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Couldn't close Storage Queue", Time_Stamp(Mode_ms));
	}

	if((Operating_Mode & Mode_Socket_Mask) == Mode_Socket_ON_Val)
	{
		if(mq_close(socket_queue) != 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_RGB!! Couldn't close Socket Queue", Time_Stamp(Mode_ms));
		}
	}

	// Logging the exit event
	syslog(LOG_INFO, "<%.6fms>!!Cam_RGB!! Exiting...", Time_Stamp(Mode_ms));

	pthread_exit(0);
}





