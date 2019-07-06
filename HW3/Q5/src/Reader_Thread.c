/*
*		File: Reader_Thread.c
*		Purpose: The source file containing implementation of reader thread for demonstrating timed Mutex Locks
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#include "main.h"

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

extern pthread_mutex_t Mutex_Locker;
extern data_strct Shared_Info;

// Following function implements Reader Thread
void *Reader_Func(void *threadp)
{

	// Time structures to use in timed_lock
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

		// Add required timeout to absolute current time
		target_time.tv_sec = curr_time.tv_sec + Mutex_Timeout;
		target_time.tv_nsec = curr_time.tv_nsec;

		// Call timed_lock
		resp = pthread_mutex_timedlock(&Mutex_Locker, &target_time);

		// Resp 0 means lock has been acquired before the timeout, so print out data, and unlock the mutex at the end
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

		// If lock timed out, then output message indicating the same
		else if(resp == ETIMEDOUT)
		{
			syslog (LOG_INFO, "<%.3fms>Reader Mutex Timed Out ***NO NEW DATA*** - Iteration(%d)", Time_Stamp(), i);
		}

		// Error handling
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
