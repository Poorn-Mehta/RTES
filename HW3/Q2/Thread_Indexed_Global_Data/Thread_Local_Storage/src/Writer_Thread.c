/*
*		File: Fib10.c
*		Purpose: The source file containing implementation of RM thread Fib10 - with overall Third Highest Priority, and 20ms of CPU Capacity
*		Owner: Poorn Mehta
*		Last Modified: 6/13/2019
*/

#include "main.h"

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

extern pthread_key_t glob_var_key;

// Following function implements Fib10 Thread
void *Writer_Func(void *threadp)
{

    data_strct writer_data;

    int *p = malloc(sizeof(int));
    *p = 1;
    pthread_setspecific(glob_var_key, p);

	uint8_t i = 0;

	while(i < 5)
	{

		syslog (LOG_INFO, "<%.3fus>Writer Started - Core(%d)", Time_Stamp(), sched_getcpu());


		writer_data.accel_x = (rand() % 100) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  writer_data.accel_x *= -1;

		writer_data.accel_y = (rand() % 100) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  writer_data.accel_y *= -1;

		writer_data.accel_z = (rand() % 100) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  writer_data.accel_z *= -1;

		writer_data.roll = (rand() % 90) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  writer_data.roll *= -1;

		writer_data.pitch = (rand() % 90) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  writer_data.pitch *= -1;

		writer_data.yaw = (rand() % 360) + ((float)(rand() % 1000) / 1000);

		clock_gettime(CLOCK_REALTIME, &writer_data.timestamp_local);

//		syslog (LOG_INFO, "Writer Wrote Accel X: %lf", writer_data.accel_x);
//		syslog (LOG_INFO, "Writer Wrote Accel Y: %lf", writer_data.accel_y);
//		syslog (LOG_INFO, "Writer Wrote Accel Z: %lf", writer_data.accel_z);
//		syslog (LOG_INFO, "Writer Wrote Roll: %lf", writer_data.roll);
//		syslog (LOG_INFO, "Writer Wrote Pitch: %lf", writer_data.pitch);
//		syslog (LOG_INFO, "Writer Wrote Yaw: %lf", writer_data.yaw);
//		syslog (LOG_INFO, "Writer Wrote Timestamp - Sec: %ld Nano Sec: %ld", writer_data.timestamp_local.tv_sec, writer_data.timestamp_local.tv_nsec);

        int* glob_spec_var = pthread_getspecific(glob_var_key);
        printf("Thread %d before mod value is %d\n", (unsigned int) pthread_self(), *glob_spec_var);
        *glob_spec_var += 1;
        printf("Thread %d after mod value is %d\n", (unsigned int) pthread_self(), *glob_spec_var);

		syslog (LOG_INFO, "<%.3fus>Writer Completed - Core(%d)", Time_Stamp(), sched_getcpu());

		i += 1;
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fus>Writer Exiting...", Time_Stamp());
	pthread_setspecific(glob_var_key, NULL);
    free(p);
	pthread_exit(0);
}
