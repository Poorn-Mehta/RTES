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

extern pthread_mutex_t Mutex_Locker;
extern data_strct Shared_Info;

// Following function implements Fib20 Thread
void *Reader_Func(void *threadp)
{

	uint8_t i = 0;

	while(i < 5)
	{

		syslog (LOG_INFO, "<%.3fus>Reader Started Waiting for Mutex to be Unlocked - Iteration(%d)", Time_Stamp(), i);

		pthread_mutex_lock(&Mutex_Locker);

		syslog (LOG_INFO, "<%.3fus>Reader Acquired Mutex - Iteration(%d)", Time_Stamp(), i);

		syslog (LOG_INFO, "<%.3fus>Reader Got Accel X: %lf", Time_Stamp(), Shared_Info.accel_x);
		syslog (LOG_INFO, "<%.3fus>Reader Got Accel Y: %lf", Time_Stamp(), Shared_Info.accel_y);
		syslog (LOG_INFO, "<%.3fus>Reader Got Accel Z: %lf", Time_Stamp(), Shared_Info.accel_z);
		syslog (LOG_INFO, "<%.3fus>Reader Got Roll: %lf", Time_Stamp(), Shared_Info.roll);
		syslog (LOG_INFO, "<%.3fus>Reader Got Pitch: %lf", Time_Stamp(), Shared_Info.pitch);
		syslog (LOG_INFO, "<%.3fus>Reader Got Yaw: %lf", Time_Stamp(), Shared_Info.yaw);
		syslog (LOG_INFO, "<%.3fus>Reader Got Timestamp - Sec: %ld Nano Sec: %ld", Time_Stamp(), Shared_Info.timestamp_local.tv_sec, Shared_Info.timestamp_local.tv_nsec);

		pthread_mutex_unlock(&Mutex_Locker);

		syslog (LOG_INFO, "<%.3fus>Reader Completed Latest Read - Iteration(%d)", Time_Stamp(), i);

		usleep(1000);

		i += 1;
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fus>Reader Exiting...", Time_Stamp());
	pthread_exit(0);
}
