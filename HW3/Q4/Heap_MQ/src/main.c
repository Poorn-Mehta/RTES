/*
*		File: main.c
*		Purpose: The source file containing main() and global variables for Question 4 of Homework 3
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*		This code is converted to Linux platform, from VxWorks source (http://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/code/VxWorks-Examples/heap_mq.c)
*  		Linux Man pages has been referred whenever necessary. Also, I have referred to my own previous work (https://github.com/Poorn-Mehta/AESD) at some points.
*/

#include "main.h"

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

// To record start time
struct timespec start_time = {0, 0};

// To terminate threads
uint8_t Terminate_Flag = 0;

// Thread handles
pthread_t Writer_Thread, Reader_Thread;

// Thread attribute variable
pthread_attr_t Attr_All;

// Scheduler parameter
struct sched_param Attr_Sch;

// Realtime Priority variables for FIFO
uint8_t FIFO_Max_Prio = 99, FIFO_Min_Prio = 1;



//info w_info, r_info;

// CPU Core Set
cpu_set_t CPU_Core;


int main (int argc, char *argv[])
{
	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

	// Setup logger
	Set_Logger("HW3_Q4_P1", LOG_DEBUG);

	// Setting up random number generator
	srand(time(0));

	syslog (LOG_INFO, ">>>>>>>>>> Program Start <<<<<<<<<<");

	// Setup the system for realtime execution, use CPU3
	if(Realtime_Setup(3) != 0)	exit(-1);

	// Give second highest priority to the Writer thread, and create it
	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Writer_Thread, &Attr_All, Writer_Func, (void *)0);
//	pthread_create(&Writer_Thread, (void *)0, Writer_Func, (void *)0);



	// Give third highest priority to the Reader thread, and create it
	Attr_Sch.sched_priority = FIFO_Max_Prio - 2;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Reader_Thread, &Attr_All, Reader_Func, (void *)0);
//	pthread_create(&Reader_Thread, (void *)0, Reader_Func, (void *)0);

	// Wait for all threads to exit
	pthread_join(Writer_Thread, 0);
	pthread_join(Reader_Thread, 0);

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Close the log
	closelog();

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);


	// Exit the program
	exit(0);
}
