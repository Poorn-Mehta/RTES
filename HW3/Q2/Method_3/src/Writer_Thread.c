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

extern pthread_mutex_t Mutex_Locker;
extern data_strct Shared_Info;

// Following function implements Fib10 Thread
void *Writer_Func(void *threadp)
{
	uint8_t i = 0;

	while(i < 5)
	{

		syslog (LOG_INFO, "<%.3fus>Writer Waiting to Acquire Mutex - Iteration(%d)", Time_Stamp(), i);

        pthread_mutex_lock(&Mutex_Locker);

        syslog (LOG_INFO, "<%.3fus>Writer Started to Generate and Update Shared Structure - Iteration(%d)", Time_Stamp(), i);

		Shared_Info.accel_x = (rand() % 100) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  Shared_Info.accel_x *= -1;
		syslog (LOG_INFO, "<%.3fus>Writer Wrote Accel X: %lf", Time_Stamp(), Shared_Info.accel_x);

		Shared_Info.accel_y = (rand() % 100) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  Shared_Info.accel_y *= -1;
		syslog (LOG_INFO, "<%.3fus>Writer Wrote Accel Y: %lf", Time_Stamp(), Shared_Info.accel_y);

		Shared_Info.accel_z = (rand() % 100) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  Shared_Info.accel_z *= -1;
		syslog (LOG_INFO, "<%.3fus>Writer Wrote Accel Z: %lf", Time_Stamp(), Shared_Info.accel_z);

		Shared_Info.roll = (rand() % 90) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  Shared_Info.roll *= -1;
		syslog (LOG_INFO, "<%.3fus>Writer Wrote Roll: %lf", Time_Stamp(), Shared_Info.roll);

		Shared_Info.pitch = (rand() % 90) + ((float)(rand() % 1000) / 1000);
		if(rand() % 2)  Shared_Info.pitch *= -1;
		syslog (LOG_INFO, "<%.3fus>Writer Wrote Pitch: %lf", Time_Stamp(), Shared_Info.pitch);

		Shared_Info.yaw = (rand() % 360) + ((float)(rand() % 1000) / 1000);
		syslog (LOG_INFO, "<%.3fus>Writer Wrote Yaw: %lf", Time_Stamp(), Shared_Info.yaw);

		clock_gettime(CLOCK_REALTIME, &Shared_Info.timestamp_local);
		syslog (LOG_INFO, "<%.3fus>Writer Wrote Timestamp - Sec: %ld Nano Sec: %ld", Time_Stamp(), Shared_Info.timestamp_local.tv_sec, Shared_Info.timestamp_local.tv_nsec);

		pthread_mutex_unlock(&Mutex_Locker);

		syslog (LOG_INFO, "<%.3fus>Writer Completed Writing New Data - Iteration(%d)", Time_Stamp(), i);

		usleep(1000);

		i += 1;
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fus>Writer Exiting...", Time_Stamp());
	pthread_exit(0);
}
