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

	struct timespec curr_time = {0, 0};
	struct timespec target_time = {0, 0};

	int resp;

	uint8_t i = 0;

	while(i < 3)
	{

        usleep(1000);

		syslog (LOG_INFO, "<%.3fms>Reader Started Waiting for Mutex to be Unlocked - Iteration(%d)", Time_Stamp(), i);

		// Record current time
        clock_gettime(CLOCK_REALTIME, &curr_time);

        target_time.tv_sec = curr_time.tv_sec + Mutex_Timeout;
        target_time.tv_nsec = curr_time.tv_nsec;

		resp = pthread_mutex_timedlock(&Mutex_Locker, &target_time);

		if(resp == 0)
		{
            syslog (LOG_INFO, "<%.3fms>Reader Acquired Mutex ***GOT NEW DATA*** - Iteration(%d)", Time_Stamp(), i);

            syslog (LOG_INFO, "<%.3fms>Reader Got Accel X: %lf", Time_Stamp(), Shared_Info.accel_x);
            syslog (LOG_INFO, "<%.3fms>Reader Got Accel Y: %lf", Time_Stamp(), Shared_Info.accel_y);
            syslog (LOG_INFO, "<%.3fms>Reader Got Accel Z: %lf", Time_Stamp(), Shared_Info.accel_z);
            syslog (LOG_INFO, "<%.3fms>Reader Got Roll: %lf", Time_Stamp(), Shared_Info.roll);
            syslog (LOG_INFO, "<%.3fms>Reader Got Pitch: %lf", Time_Stamp(), Shared_Info.pitch);
            syslog (LOG_INFO, "<%.3fms>Reader Got Yaw: %lf", Time_Stamp(), Shared_Info.yaw);
            syslog (LOG_INFO, "<%.3fms>Reader Got Timestamp - Sec: %ld Nano Sec: %ld", Time_Stamp(), Shared_Info.timestamp_local.tv_sec, Shared_Info.timestamp_local.tv_nsec);

            pthread_mutex_unlock(&Mutex_Locker);

            i += 1;
		}

		else if(resp == ETIMEDOUT)
		{
            syslog (LOG_INFO, "<%.3fms>Reader Mutex Timed Out ***NO NEW DATA*** - Iteration(%d)", Time_Stamp(), i);
		}

		else
		{
            syslog(LOG_ERR, "<%.3fms>Reader Timed Mutex Lock Resulted in Error - Iteration(%d)", Time_Stamp(), i);
            pthread_exit(0);
		}

		syslog (LOG_INFO, "<%.3fms>Reader Completed Latest Read - Iteration(%d)", Time_Stamp(), i);
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fms>Reader Exiting...", Time_Stamp());
	pthread_exit(0);
}
