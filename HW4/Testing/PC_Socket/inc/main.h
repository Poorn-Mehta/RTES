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

#define No_of_Cores	6

#define Mode_sec	1
#define Mode_ms		2
#define Mode_us		3

#define Port_Num		8080
#define Default_IP		"192.168.50.122"

#define Mode_Infinite		1
#define Mode_Limited		0
#define Max_Retries		10
#define Socket_Retry_Mode	Mode_Limited

#define Socket_Timeout_sec	5
#define Listen_Queue_Length	10

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

#define Wrap_around_Frames	101
#define No_of_Frames        6001

#define No_of_Buffers	    100
#define Big_Buffer_Size     640*480*3

// To set passed strcuture to 0
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define Filename_Len	40
#define Header_Len	110

#define Segment_Size	1024

typedef struct
{
	char filename[Filename_Len];
	char header[Header_Len];
	uint8_t *dataptr;
	uint32_t headersize;
	uint32_t filesize;
} store_struct;

float Time_Stamp(uint8_t mode);
void Set_Logger(char *logname, int level_upto);
uint8_t Bind_to_CPU(uint8_t Core);
uint8_t Realtime_Setup(void);

void *Socket_Func(void *para_t);

#endif
