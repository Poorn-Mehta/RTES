/*
*		File: Fib20.c
*		Purpose: The source file containing implementation of RM thread Fib20 - with overall Third Highest Priority, and 20ms of CPU Capacity
*		Owner: Poorn Mehta
*		Last Modified: 6/13/2019
*/

#include "main.h"

// Shared Variables
extern sem_t Fib20_Sem;
extern uint8_t Terminate_Flag;
extern uint32_t Fib_Loops_for_20ms;

// Following function implements Fib20 Thread
void *Fib20_Func(void *threadp)
{
	while(1)
	{
		// Lock the thread till semaphore is released
		sem_wait(&Fib20_Sem);

		// Check whether it should terminate or not
		if(Terminate_Flag != 0)		break;
	
		// Following piece of code calibrates load on runtime
		uint64_t temp_time;
		struct timespec Temp1 = {0, 0};
		struct timespec Temp2 = {0, 0};

		// Do while loop to avoid condition where tv_sec has changed
		do
		{
			clock_gettime(CLOCK_REALTIME, &Temp1);
			Fib_Calc(1000);
			clock_gettime(CLOCK_REALTIME, &Temp2);
		}while(Temp2.tv_nsec < Temp1.tv_nsec);
		temp_time = (Temp2.tv_nsec - Temp1.tv_nsec) / 1000;

		// Calculating loops for 18ms, so 2ms can be accounted for a few things contributing to WCET
		Fib_Loops_for_20ms = (uint32_t)((uint64_t)18000000 / (uint64_t)temp_time);	
		
		// Timestamping and Logging the starting of load calculation
		syslog (LOG_INFO, "<%.3fms>Fib20 Started - Core(%d)", Time_Stamp(), sched_getcpu());

		// Calculate for 20ms
		Fib_Calc(Fib_Loops_for_20ms);

		// Timestamping and Logging the completion of load calculation
		syslog (LOG_INFO, "<%.3fms>Fib20 Completed - Core(%d)", Time_Stamp(), sched_getcpu());
	}
	
	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fms>Fib20 Exiting...", Time_Stamp());
	Terminate_Flag += 1;
	pthread_exit(0);
}
