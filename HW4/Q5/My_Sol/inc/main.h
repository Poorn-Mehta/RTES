/*
*		File: main.h
*		Purpose: The header file containing useful libraries, global defines, and function prototypes
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#ifndef	__MAIN_H__
#define __MAIN_H__

#define _GNU_SOURCE

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>

#include <syslog.h>
#include <inttypes.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <assert.h>
#include <getopt.h>

#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <signal.h>

// V4L2 Library
#include <linux/videodev2.h>

#define No_of_Cores	4

#define Mode_sec	1
#define Mode_ms		2
#define Mode_us		3

#define Logging_Msg_Len     150
#define Source_Name_Len     20

#define logger_q_name 		"/log_q"
#define grey_q_name 		"/grey_q"
#define bright_q_name 		"/bright_q"
#define contrast_q_name 	"/contrast_q"
#define monitor_q_name 		"/monitor_q"
#define queue_size		100

//#define HRES 800
//#define VRES 600
#define HRES_STR "960"
#define VRES_STR "720"

#define Failed_State	0
#define Success_State	1

#define Target_FPS          10

#define No_of_Frames        1001

#define Big_Buffer_Size     960*720

#define Margin_Factor	(float)0.95

#define Brightness_Factor	1.8

// To set passed strcuture to 0
#define CLEAR(x) memset(&(x), 0, sizeof(x))

typedef enum
{
	Log_Main,
	Log_Logger,
	Log_Monitor,
	Log_Bright,
	Log_Contrast,
	Log_Grey,
	Log_Socket,
	Log_Scheduler
} log_sources;

typedef struct
{
	uint8_t Grey_status;
	uint8_t Bright_status;
	uint8_t Contrast_status;
} monitor_struct;

typedef struct
{
	char message[Logging_Msg_Len];
	int log_level;
	log_sources source;
} log_struct;

typedef enum
{
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR
} io_options;

typedef struct
{
	void   *start;
	size_t  length;
} frame_p_buffer;

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

void *Logger_Func(void *para_t);
void *Cam_Monitor_Func(void *para_t);
void *Cam_Grey_Func(void *para_t);
void *Cam_Brightness_Func(void *para_t);
void *Cam_Contrast_Func(void *para_t);

#endif
