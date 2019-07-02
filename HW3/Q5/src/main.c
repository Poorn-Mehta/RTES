/*
*		File: main.c
*		Purpose: The source file containing main() and global variables for Question 4 of Homework 1
*		Owner: Poorn Mehta
*		Last Modified: 6/13/2019
*/

/*
*	This code has been completely written by me, however - with the help of following resources
*
*	>> Exmaples and codes provided by Professor Sam Siewert
*	(1) http://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/simplethread/
*	(2) http://ecee.colorado.edu/%7Eecen5623/ecen/ex/Linux/incdecthread/pthread.c
*	(3) http://ecee.colorado.edu/%7Eecen5623/ecen/ex/Linux/example-3/testdigest.c
*	(4) http://ecee.colorado.edu/%7Eecen5623/ecen/ex/Linux/code/VxWorks-sequencers/
*
*	>> External resources
*	(5) http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
*	(6) Linux man pages
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

pthread_mutex_t Mutex_Locker;

data_strct Shared_Info;

//info w_info, r_info;

// CPU Core Set
cpu_set_t CPU_Core;


int main (int argc, char *argv[])
{
	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

	// Setup logger
	Set_Logger("HW3_Q5", LOG_DEBUG);

	srand(time(0));

	syslog (LOG_INFO, ">>>>>>>>>> Program Start <<<<<<<<<<");

	if(pthread_mutex_init(&Mutex_Locker, (void *)0))
	{
        syslog(LOG_ERR, "Failed to Initialize Mutex");
		return 1;
	}

	srand(time(0));

    Shared_Info.accel_x = 0;
    Shared_Info.accel_y = 0;
    Shared_Info.accel_z = 0;
    Shared_Info.roll = 0;
    Shared_Info.pitch = 0;
    Shared_Info.yaw = 0;
    clock_gettime(CLOCK_REALTIME, &Shared_Info.timestamp_local);

	// Setup the system for realtime execution, use CPU3
//	if(Realtime_Setup(3) != 0)	exit(-1);

	// Give second highest priority to the Fib10 thread, and create it
//	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
//	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
//	pthread_create(&Writer_Thread, &Attr_All, Writer_Func, (void *)0);
	pthread_create(&Writer_Thread, (void *)0, Writer_Func, (void *)0);

	usleep(100);

	// Give third highest priority to the Fib20 thread, and create it
//	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
//	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
//	pthread_create(&Reader_Thread, &Attr_All, Reader_Func, (void *)0);
	pthread_create(&Reader_Thread, (void *)0, Reader_Func, (void *)0);

	// Wait for all threads to exit
	pthread_join(Writer_Thread, 0);
	pthread_join(Reader_Thread, 0);

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Close the log
	closelog();

	pthread_mutex_destroy(&Mutex_Locker);

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);


	// Exit the program
	exit(0);
}
