/*
*		File: main.h
*		Purpose: The header file containing useful libraries, global defines, and function prototypes
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#ifndef	__MAIN_H__
#define __MAIN_H__

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#include <syslog.h>

#include <semaphore.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <netdb.h>
#include <malloc.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include <netinet/in.h>
#include <arpa/inet.h>

// V4L2 Library
#include <linux/videodev2.h>

#define No_of_Cores	4

#define Mode_sec	1
#define Mode_ms		2
#define Mode_us		3

#define Logging_Msg_Len     150
#define Source_Name_Len     20

#define storage_q_name 		"/store_q"
#define socket_q_name 		"/socket_q"
#define queue_size		100

#define Port_Num		8080
#define Default_IP		"192.168.50.104"

#define Mode_Infinite		1
#define Mode_Limited		0
#define Max_Retries		10
#define Socket_Retry_Mode	Mode_Limited

#define Socket_Timeout_sec	2

#define Target_FPS          10

#define Deadline_ms		(uint32_t)(1000 / Target_FPS)
#define s_to_ns			(uint32_t)1000000000
#define s_to_us			(uint32_t)1000000
#define s_to_ms			(uint32_t)1000
#define ms_to_ns		(uint32_t)1000000
#define ms_to_us		(uint32_t)1000
#define us_to_ns		(uint32_t)1000
#define ns_to_us		(float)0.001
#define ns_to_ms		(float)0.000001
#define ns_to_s			(float)0.000000001
#define No_Frame_Max_us		(uint32_t)250
#define Select_Wait_ms		(uint32_t)10
#define Monitor_Sleep_ms	(uint32_t)20

#define Iterations		(uint8_t)1

#define Mutex_Timeout_ns	(uint32_t)25000000

#define Grey_Complete_Mask	(uint8_t)(1 << 0)
#define Brightness_Complete_Mask	(uint8_t)(1 << 1)
#define Contrast_Complete_Mask		(uint8_t)(1 << 2)
#define Process_Complete_Mask		(uint8_t)(1 << 3)
#define Total_Complete		(uint8_t)(Grey_Complete_Mask | Brightness_Complete_Mask | Contrast_Complete_Mask)

//#define HRES 800
//#define VRES 600
#define HRES_STR "640"
#define VRES_STR "480"

#define Failed_State	0
#define Success_State	1

#define Wrap_around_Frames	101
#define No_of_Frames        6001

#define No_of_Buffers	    100
#define Big_Buffer_Size     640*480*3

#define Margin_Factor	(float)0.95

#define Brightness_Factor	1.8

// To set passed strcuture to 0
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define Filename_Len		40
#define Header_Len		110

#define Segment_Size		1024
#define Frame_Socket_Max_Retries	10

typedef struct
{
	void   *start;
	size_t  length;
} frame_p_buffer;

typedef struct
{
	char filename[Filename_Len];
	char header[Header_Len];
	uint8_t *dataptr;
	uint32_t headersize;
	uint32_t filesize;
} store_struct;

extern struct v4l2_format fmt;

float Time_Stamp(uint8_t mode);
void Set_Logger(char *logname, int level_upto);
uint8_t Bind_to_CPU(uint8_t Core);
uint8_t Realtime_Setup(void);

void errno_exit(const char *s);
int xioctl(int fh, int request, void *arg);
void stop_capturing(void);
void start_capturing(void);
void uninit_device(void);
void init_userp(uint32_t buffer_size);
void init_device(void);
void close_device(void);
void open_device(void);

uint8_t read_frame(void);
int device_warmup(void);

void *Cam_Monitor_Func(void *para_t);
void *Cam_Brightness_Func(void *para_t);
void *Storage_Func(void *para_t);
void *Socket_Func(void *para_t);
void *Scheduler_Func(void *para_t);

#endif
