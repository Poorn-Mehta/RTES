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

// To record start time
struct timespec start_time = {0, 0};

// To terminate threads
uint8_t Terminate_Flag = 0;

// Thread handles
pthread_t Fib10, Fib20, Fib_Sch;

// Semaphores for threads
sem_t Fib10_Sem, Fib20_Sem;

// Thread attribute variable
pthread_attr_t Attr_All;

// Scheduler parameter
struct sched_param Attr_Sch;

// Realtime Priority variables for FIFO
uint8_t FIFO_Max_Prio = 99, FIFO_Min_Prio = 1;

// Loop count variables 
uint32_t Fib_Loops_for_10ms, Fib_Loops_for_20ms;

// CPU Core Set
cpu_set_t CPU_Core;


int main (int argc, char *argv[])
{
	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

	// Setup logger
	Set_Logger("HW1_Q4_Log", LOG_DEBUG);

	// Setup the system for realtime execution, use CPU3
	if(Realtime_Setup(3) != 0)	exit(-1);

	// Initiazlie semaphore for Fib10 thread, to 0
	if(sem_init(&Fib10_Sem, 0, 0) != 0)
	{
	    syslog(LOG_ERR, "Failed to initialize Fib10 semaphore");
	    exit(-1);
	}

	// Initiazlie semaphore for Fib20 thread, to 0
	if(sem_init(&Fib20_Sem, 0, 0) != 0)
	{
	    syslog(LOG_ERR, "Failed to initialize Fib20 semaphore");
	    exit(-1);
	}

	// Give highest priority to the scheduling thread, and create it
	Attr_Sch.sched_priority = FIFO_Max_Prio;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Fib_Sch, &Attr_All, Fib_Scheduler, (void *)0);

	// Give second highest priority to the Fib10 thread, and create it
	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Fib10, &Attr_All, Fib10_Func, (void *)0);

	// Give third highest priority to the Fib20 thread, and create it
	Attr_Sch.sched_priority = FIFO_Max_Prio - 2;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Fib20, &Attr_All, Fib20_Func, (void *)0); 

	// Wait for all threads to exit
	pthread_join(Fib10, 0);
	pthread_join(Fib20, 0);
	pthread_join(Fib_Sch, 0);

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Close the log
	closelog();

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);

	// Destroy semaphores
	sem_destroy(&Fib10_Sem);
	sem_destroy(&Fib20_Sem);

	// Exit the program
	exit(0);
}
