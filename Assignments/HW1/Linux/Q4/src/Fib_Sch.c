/*
*		File: Fib_Sch.c
*		Purpose: The source file containing scheduler function for Fib10 and Fib20 threads
*		Owner: Poorn Mehta
*		Last Modified: 6/13/2019
*/

#include "main.h"

// Shared variables
extern sem_t Fib10_Sem, Fib20_Sem;
extern uint8_t Terminate_Flag;

// Function to implement scheduling thread
void *Fib_Scheduler(void *threadp)
{
	uint8_t j;
	// How many LCM periods - to run for
	for(j = 0; j < No_of_LCM; j ++)
	{
		// Implementing RM policy with the use of microsleep and semaphores
		// Requestig both at the same time, Critical Instant
		sem_post(&Fib10_Sem);
		syslog (LOG_INFO, "<%.3fms>Sem Released: Fib10", Time_Stamp());
		sem_post(&Fib20_Sem);
		syslog (LOG_INFO, "<%.3fms>Sem Released: Fib20", Time_Stamp());
		usleep(20000);
		
		// Period of Fib10 has arrived, request again
		sem_post(&Fib10_Sem);
		syslog (LOG_INFO, "<%.3fms>Sem Released: Fib10", Time_Stamp());
		usleep(20000);

		// Fib20 has just completed processing. Period of Fib10 has reached, request again. 
		sem_post(&Fib10_Sem);
		syslog (LOG_INFO, "<%.3fms>Sem Released: Fib10", Time_Stamp());
		usleep(10000);

		// Period of Fib20 has arrived, request again
		sem_post(&Fib20_Sem);
		syslog (LOG_INFO, "<%.3fms>Sem Released: Fib20", Time_Stamp());
		usleep(10000);

		// Periodic request for Fib10
		sem_post(&Fib10_Sem);
		syslog (LOG_INFO, "<%.3fms>Sem Released: Fib10", Time_Stamp());
		usleep(20000);

		// Final request for Fib10. Fib20 has been completed 2 times now. 
		sem_post(&Fib10_Sem);
		syslog (LOG_INFO, "<%.3fms>Sem Released: Fib10", Time_Stamp());
		usleep(20000);
	}

	// Terminate Both Fib10 and Fib20 Threads
	Terminate_Flag = 1;

	// Release Fib10 so that it can exit
	sem_post(&Fib10_Sem);
	usleep(1000);

	// Check and Timestamp the exit status of Fib10
	if(Terminate_Flag == 2)		syslog(LOG_INFO, "<%.3fms>Fib10 Exited...", Time_Stamp());
	else	syslog(LOG_ERR, "<%.3fms>Fib10 Exit Failed!", Time_Stamp());

	// Release Fib20 so that it can exit
	sem_post(&Fib20_Sem);
	usleep(1000);

	// Check and Timestamp the exit status of Fib20
	if(Terminate_Flag == 3)		syslog(LOG_INFO, "<%.3fms>Fib20 Exited...", Time_Stamp());
	else	syslog(LOG_ERR, "<%.3fms>Fib20 Exit Failed!", Time_Stamp());

	// Terminate self
	pthread_exit(0);
}
