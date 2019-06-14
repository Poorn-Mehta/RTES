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

#define No_of_Cores	4
#define No_of_LCM	1

float Time_Stamp(void);
void Set_Logger(char *logname, int level_upto);
uint8_t Realtime_Setup(uint8_t Bind_to_CPU);

void Fib_Calc(uint32_t loops);
void Dly_ms(uint8_t req_ms);

void *Fib10_Func(void *threadp);
void *Fib20_Func(void *threadp);
void *Fib_Scheduler(void *threadp);

#endif
