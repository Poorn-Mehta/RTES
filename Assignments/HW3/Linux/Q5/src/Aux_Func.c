/*
*		File: Aux_Func.c
*		Purpose: The source file containing Auxiliary functions related to the functionalities required
*		Owner: Poorn Mehta
*		Last Modified: 6/13/2019
*/

#include "main.h"

// Shared variables
extern struct timespec start_time;
extern pthread_attr_t Attr_All;
extern struct sched_param Attr_Sch;
extern uint8_t FIFO_Max_Prio, FIFO_Min_Prio;
extern cpu_set_t CPU_Core;

// Function to return relative timestamps
float Time_Stamp(void)
{
	struct timespec curr_time = {0, 0};

	// Record current time
	clock_gettime(CLOCK_REALTIME, &curr_time);
	float rval = 0;

	// Calculate Difference
	if(curr_time.tv_nsec >= start_time.tv_nsec)
		rval = ((curr_time.tv_sec - start_time.tv_sec) * 1000) + (((float)(curr_time.tv_nsec - start_time.tv_nsec)) / 1000000);
	else
		rval = ((curr_time.tv_sec - start_time.tv_sec - 1) * 1000) + (((float)(1000000000 - (start_time.tv_nsec - curr_time.tv_nsec))) / 1000000);

	return (rval);
}

// Setup Logger as needed
void Set_Logger(char *logname, int level_upto)
{
	// Logmask - the lowest (least important) Log Level to be written
	setlogmask(LOG_UPTO(level_upto));

	// Open log with prefix name
	openlog(logname, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
}

// Setting up system for realtime operation
uint8_t Realtime_Setup(uint8_t Bind_to_CPU)
{
	int sched;

	// Initializing Attribute which will be passed to all pthreads
	if(pthread_attr_init(&Attr_All) != 0)
	{
		syslog(LOG_ERR, "Failed to Initialize Attr_All");
		return 1;
	}

	// Preventing pthreads to inherit scheduler, and tell them explicity to use given scheduler (through another API)
	if(pthread_attr_setinheritsched(&Attr_All, PTHREAD_EXPLICIT_SCHED) != 0)
	{
		syslog(LOG_ERR, "Failed to Set Schedule Inheritance");
		return 1;
	}

	// Set scheduling policy to FIFO for all threads
	if(pthread_attr_setschedpolicy(&Attr_All, SCHED_FIFO) != 0)
	{
		syslog(LOG_ERR, "Failed to Set Scheduling Policy");
		return 1;
	}

	// Bind all threads to a single CPU core
	if(Bind_to_CPU < No_of_Cores)
	{
		// First, clearing variable
		CPU_ZERO(&CPU_Core);

		// Adding core-<Bind_to_CPU> in the variable
		CPU_SET(Bind_to_CPU, &CPU_Core);

		// Executing API to set the CPU
		if(pthread_attr_setaffinity_np(&Attr_All, sizeof(cpu_set_t), &CPU_Core) != 0)
		{
			syslog(LOG_ERR, "Failed to Set CPU Core");
			return 1;
		}
	}

	// Getting Max and Min realtime priorities for FIFO
	FIFO_Max_Prio = sched_get_priority_max(SCHED_FIFO);
	FIFO_Min_Prio = sched_get_priority_min(SCHED_FIFO);

	// Setting scheduler for the program (by setting parent thread to FIFO) (I am not 100% sure about this though)
	Attr_Sch.sched_priority = FIFO_Max_Prio;
	if(sched_setscheduler(getpid(), SCHED_FIFO, &Attr_Sch) != 0)
	{
		perror("sched_setscheduler");
		syslog(LOG_ERR, "Failed to Set FIFO Scheduler");
		return 1;
	}

	// Verify that Scheduler is set to FIFO
	sched = sched_getscheduler(getpid());
	if(sched == SCHED_FIFO)		syslog (LOG_INFO, "Scheduling set to FIFO");
	else
	{
		syslog(LOG_ERR, "Scheduling is not FIFO");
		return 1;
	}

	return 0;

}
