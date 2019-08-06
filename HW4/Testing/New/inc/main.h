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
#include <sys/utsname.h>

#include <netinet/in.h>
#include <arpa/inet.h>

// V4L2 Library
#include <linux/videodev2.h>

#define No_of_Cores	(uint8_t)4

#define Mode_sec	(uint8_t)1
#define Mode_ms		(uint8_t)2
#define Mode_us		(uint8_t)3

#define Logging_Msg_Len     (uint32_t)150
#define Source_Name_Len     (uint32_t)20

#define storage_q_name 		"/store_q"
#define socket_q_name 		"/socket_q"
#define queue_size		(uint8_t)100

#define Port_Num		(uint32_t)8080
#define Default_IP		"192.168.50.104"

#define Mode_Infinite		(uint8_t)1
#define Mode_Limited		(uint8_t)0
#define Max_Retries		(uint8_t)10
#define Socket_Retry_Mode	Mode_Limited

#define Socket_Timeout_sec	(uint8_t)2

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

#define Deadline_Overhead_Factor	(float)1.01

#define Iterations		(uint8_t)1

#define Mutex_Timeout_ns	(uint32_t)25000000

#define Grey_Complete_Mask		(uint8_t)(1 << 0)
#define Brightness_Complete_Mask	(uint8_t)(1 << 1)
#define Contrast_Complete_Mask		(uint8_t)(1 << 2)
#define Process_Complete_Mask		(uint8_t)(1 << 3)
#define Total_Complete			(uint8_t)(Grey_Complete_Mask | Brightness_Complete_Mask | Contrast_Complete_Mask)

#define IP_Addr_Len		(uint8_t)20

#define Mode_FPS_Pos		(uint8_t)0
#define Mode_FPS_Mask		(uint8_t)(1 << Mode_FPS_Pos)
#define Mode_1_FPS		"FPS_1"
#define Mode_10_FPS		"FPS_10"
#define Mode_1_FPS_Val		(uint8_t)0
#define Mode_10_FPS_Val		(uint8_t)1
#define Mode_Socket_Pos		(uint8_t)1
#define Mode_Socket_Mask		(uint8_t)(1 << Mode_Socket_Pos)
#define Mode_Socket_OFF		"OFF"
#define Mode_Socket_ON		"ON"
#define Mode_Socket_OFF_Val		(uint8_t)(0 << Mode_Socket_Pos)
#define Mode_Socket_ON_Val		(uint8_t)(1 << Mode_Socket_Pos)

//#define HRES 800
//#define VRES 600
#define HRES_STR "640"
#define VRES_STR "480"

#define Failed_State	(uint8_t)0
#define Success_State	(uint8_t)1

#define No_of_Buffers	    (uint32_t)100
#define Big_Buffer_Size     (uint32_t)(640*480*3)

#define Margin_Factor	(float)0.95

#define Brightness_Factor	(float)1.8

// To set passed strcuture to 0
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define Filename_Len		(uint32_t)40
#define Header_Len		(uint32_t)110

#define Segment_Size		(uint32_t)1024
#define Frame_Socket_Max_Retries	(uint8_t)10

#define Useless_Frames		(uint32_t)2
#define Test_Frames		(uint32_t)90

#define Frame_to_Capture	(uint32_t)61

#define Wrap_around_Frames	(uint32_t)601
#define No_of_Frames        (uint32_t)(Frame_to_Capture + Useless_Frames)

#define Warmup_Frames		(uint32_t)100

#define Pix_Allowed_Val		(uint8_t)6
#define Pix_Max_Val		(uint8_t)255
#define Pix_Min_Val		(uint8_t)0

#define Diff_Thr_Low		(float)(0.1)
#define Diff_Thr_High		(float)(0.4)

#define Offset_ms		(uint32_t)(500)

#define No_of_Threads		(uint8_t)5

#define Scheduler_TID		(uint8_t)0
#define Monitor_TID		(uint8_t)1
#define Brightness_TID		(uint8_t)2
#define Storage_TID		(uint8_t)3
#define Socket_TID		(uint8_t)4

#define No_of_CPU_Utilized	(uint8_t)3

#define Scheduler_Scaling_Factor	(uint32_t)100
#define Scheduler_Loop_Count		(uint32_t)(Scheduler_Scaling_Factor * No_of_Frames * 2)

#define Monitor_Scaling_Factor		(uint32_t)10
#define Monitor_Loop_Count		(uint32_t)(Monitor_Scaling_Factor * No_of_Frames)

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
	uint32_t total_frames;
} store_struct;

typedef struct
{
	float Scheduler_Jitter[Scheduler_Loop_Count];
	float Monitor_Jitter[Monitor_Loop_Count]; 
	float Brightness_Jitter[No_of_Frames];
	float Storage_Jitter[No_of_Frames];
	float Socket_Jitter[No_of_Frames];
	float Overall_Jitter[No_of_Threads];
	float Avg_Jitter[No_of_Threads];
	float Max_Jitter[No_of_Threads];
} strct_jitter;

typedef struct
{
	float Scheduler_Exec[Scheduler_Loop_Count];
	float Monitor_Exec[Monitor_Loop_Count]; 
	float Brightness_Exec[No_of_Frames];
	float Storage_Exec[No_of_Frames];
	float Socket_Exec[No_of_Frames];
	float Avg_Exec[No_of_Threads];
	float WCET[No_of_Threads];
} strct_exec;

typedef struct
{
	float Max_FPS;
	float Running_FPS;
	uint32_t Missed_Deadlines;
	strct_jitter Jitter_Analysis;
	strct_exec Exec_Analysis;
	float CPU_Loading[No_of_CPU_Utilized];
} strct_analyze;

extern struct v4l2_format fmt;

void Show_Analysis(void);
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
uint8_t device_warmup(void);

void Cam_Filter(void);

void *Cam_Monitor_Func(void *para_t);
void *Cam_Brightness_Func(void *para_t);
void *Storage_Func(void *para_t);
void *Socket_Func(void *para_t);
void *Scheduler_Func(void *para_t);

#endif
