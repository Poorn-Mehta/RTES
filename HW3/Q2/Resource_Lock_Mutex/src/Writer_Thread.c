/*
*		File: Writer_Thread.c
*		Purpose: The source file containing implementation of writer thread for demonstrating mutex locks for resource sharing and synchronization
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/
#include "main.h"

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

// Shared variables
extern pthread_mutex_t Mutex_Locker;
extern data_strct Shared_Info;

// This function implements Writer Function
void *Writer_Func(void *threadp)
{
	uint8_t i = 0;

	while(i < 4)
	{

		syslog (LOG_INFO, "<%.3fus>Writer Waiting to Acquire Mutex - Iteration(%d)", Time_Stamp(), i);

		// Waiting for Mutex
		pthread_mutex_lock(&Mutex_Locker);

		// Mutex acquired and locked. Generating random data and updating global shared data structure.
		// Printing the generated data as well along with relative time stamps for verification.
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

		syslog (LOG_INFO, "<%.3fus>Writer Completed Writing New Data - Iteration(%d)", Time_Stamp(), i);

		// Releasing Mutex
		pthread_mutex_unlock(&Mutex_Locker);

		// This sleep is necessary - otherwise intense locking-unlocking won't let other thread run
		usleep(100000);

		i += 1;
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fus>Writer Exiting...", Time_Stamp());
	pthread_exit(0);
}
