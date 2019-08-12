/*
*		File: main.h
*		Purpose: The header file includes necessary libraries, defines globally used structures, and variables
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
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

// POSIX Queue Defines
#define storage_q_name 		"/store_q"
#define socket_q_name 		"/socket_q"
#define queue_size		(uint8_t)121

// Input mode defines (using command line arguments)
#define Mode_FPS_Pos		(uint8_t)0
#define Mode_FPS_Mask		(uint8_t)(1 << Mode_FPS_Pos)
#define Mode_1_FPS		"FPS_1"
#define Mode_10_FPS		"FPS_10"
#define Mode_1_FPS_Val		(uint8_t)0
#define Mode_10_FPS_Val		(uint8_t)1
#define Mode_Socket_Pos		(uint8_t)1
#define Mode_Socket_Mask	(uint8_t)(1 << Mode_Socket_Pos)
#define Mode_Socket_OFF		"OFF"
#define Mode_Socket_ON		"ON"
#define Mode_Socket_OFF_Val	(uint8_t)(0 << Mode_Socket_Pos)
#define Mode_Socket_ON_Val	(uint8_t)(1 << Mode_Socket_Pos)

// Circular Buffer Size
#define No_of_Buffers	    	(uint32_t)100

// Size of RGB frame (each pixel has 3 bytes - RGB)
#define Big_Buffer_Size     	(uint32_t)(640*480*3)

// To set passed strcuture to 0
#define CLEAR(x) memset(&(x), 0, sizeof(x))

// Used for array size
#define Filename_Len		(uint32_t)40
#define Header_Len		(uint32_t)210

// Frames that are to be masked after all initialization - mostly to flush the buffer of size 2 in mmap
#define Useless_Frames		(uint32_t)2

// The number of total frames that are to be captured
#define Frame_to_Capture	(uint32_t)6001

// The number of frames that are allowed to be stored on local flash (the latest frames will be there in case of Frames_to_Capture > Wrap_around_Frames)
#define Wrap_around_Frames	(uint32_t)2001

// This is used by program - to be compared with internal frame counter
#define No_of_Frames        (uint32_t)(Frame_to_Capture + Useless_Frames)

// The number of frames taken initially, at maximum FPS, and simply discarded
#define Warmup_Frames		(uint32_t)100

// Used for analysis
#define No_of_Threads		(uint8_t)5

#define Scheduler_TID		(uint8_t)0
#define Monitor_TID		(uint8_t)1
#define RGB_TID			(uint8_t)2
#define Storage_TID		(uint8_t)3
#define Socket_TID		(uint8_t)4

#define No_of_CPU_Utilized	(uint8_t)3

#define Scheduler_Scaling_Factor	(uint32_t)100
#define Scheduler_Loop_Count		(uint32_t)(Scheduler_Scaling_Factor * No_of_Frames * 2)

#define Monitor_Scaling_Factor		(uint32_t)1
#define Monitor_Loop_Count		(uint32_t)(Monitor_Scaling_Factor * No_of_Frames)

// All custom structures used in the project

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
	float RGB_Jitter[No_of_Frames];
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
	float RGB_Exec[No_of_Frames];
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


// All shared variables

extern struct timespec start_time;
extern struct sched_param Attr_Sch;
extern struct utsname sys_info;
extern struct v4l2_format fmt;
extern struct timespec prev_t;
extern strct_analyze Analysis;
extern frame_p_buffer shared_struct, *frame_p;

extern pthread_attr_t Attr_All;
extern cpu_set_t CPU_Core;
extern sem_t Sched_Sem, RGB_Sem, Monitor_Sem;;

extern char target_ip[20], *dev_name;

extern uint8_t FIFO_Max_Prio, FIFO_Min_Prio, data_buffer[No_of_Buffers][Big_Buffer_Size];
extern uint8_t Terminate_Flag, Operating_Mode, Complete_Var;

extern int fd,force_format;

extern uint32_t Target_FPS, Deadline_ms, Scheduler_Deadline;
extern uint32_t Monitor_Deadline, sch_index, n_buffers, HRES, VRES;

extern float RGB_Start_Stamp, RGB_Stamp_1, Monitor_Start_Stamp, Monitor_Stamp_1;

#endif
