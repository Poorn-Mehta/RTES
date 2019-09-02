/*
*		File: Fib20.c
*		Purpose: The source file containing implementation of RM thread Fib20 - with overall Third Highest Priority, and 20ms of CPU Capacity
*		Owner: Poorn Mehta
*		Last Modified: 6/13/2019
*/

#include "main.h"

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

extern pthread_key_t glob_var_key;

// Following function implements Fib20 Thread
void *Reader_Func(void *threadp)
{

    data_strct reader_data;

    int *p = malloc(sizeof(int));
    *p = 1;
    pthread_setspecific(glob_var_key, p);

	uint8_t i = 0;

	while(i < 5)
	{

		syslog (LOG_INFO, "<%.3fus>Reader Started - Core(%d)", Time_Stamp(), sched_getcpu());

//		syslog (LOG_INFO, "Reader Got Accel X: %lf", reader_data.accel_x);
//		syslog (LOG_INFO, "Reader Got Accel Y: %lf", reader_data.accel_y);
//		syslog (LOG_INFO, "Reader Got Accel Z: %lf", reader_data.accel_z);
//		syslog (LOG_INFO, "Reader Got Roll: %lf", reader_data.roll);
//		syslog (LOG_INFO, "Reader Got Pitch: %lf", reader_data.pitch);
//		syslog (LOG_INFO, "Reader Got Yaw: %lf", reader_data.yaw);
//		syslog (LOG_INFO, "Reader Got Timestamp - Sec: %ld Nano Sec: %ld", reader_data.timestamp_local.tv_sec, reader_data.timestamp_local.tv_nsec);

        int* glob_spec_var = pthread_getspecific(glob_var_key);
        printf("Thread %d read mod value is %d\n", (unsigned int) pthread_self(), *glob_spec_var);

		syslog (LOG_INFO, "<%.3fus>Reader Completed - Core(%d)", Time_Stamp(), sched_getcpu());

		i += 1;
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fus>Reader Exiting...", Time_Stamp());
	pthread_setspecific(glob_var_key, NULL);
    free(p);
	pthread_exit(0);
}
