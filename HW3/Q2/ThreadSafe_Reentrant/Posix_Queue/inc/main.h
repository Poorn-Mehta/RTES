/*
*		File: main.h
*		Purpose: The header file containing useful libraries, defines, and function prototypes
*		Owner: Poorn Mehta
*		Last Modified: 6/13/2019
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

#define No_of_Cores	4

#define queue_name 		"/custom_q"
#define queue_size		10

// data structure
typedef struct
{
  double accel_x;
  double accel_y;
  double accel_z;
  double roll;
  double pitch;
  double yaw;
  struct timespec timestamp_local;
}data_strct;

float Time_Stamp(void);
void Set_Logger(char *logname, int level_upto);
uint8_t Realtime_Setup(uint8_t Bind_to_CPU);

void *Writer_Func(void *threadp);
void *Reader_Func(void *threadp);

#endif
