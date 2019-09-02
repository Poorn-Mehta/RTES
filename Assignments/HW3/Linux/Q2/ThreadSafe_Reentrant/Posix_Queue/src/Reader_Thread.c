/*
*		File: Reader_Thread.c
*		Purpose: 
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#include "main.h"

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

void *Reader_Func(void *threadp)
{

    data_strct reader_data;

	// Queue setup
	mqd_t custom_queue;
	struct mq_attr custom_queue_attr;
	custom_queue_attr.mq_maxmsg = queue_size;
	custom_queue_attr.mq_msgsize = sizeof(reader_data);

	custom_queue = mq_open(queue_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &custom_queue_attr);
	if(custom_queue == -1)
	{
		syslog(LOG_ERR, "\nQueue opening error for Reader");
		pthread_exit(0);
	}

	int q_recv_resp;

	uint8_t i = 0;

	while(i < 1)
	{

		syslog (LOG_INFO, "<%.3fus>Reader Started - Core(%d)", Time_Stamp(), sched_getcpu());

		q_recv_resp = mq_receive(custom_queue,&reader_data,sizeof(data_strct),0);

        if(q_recv_resp < 0)
        {
            syslog(LOG_ERR, "\nQueue receiving error for Reader");
            printf("\nError Code: %d", q_recv_resp);
            perror("\nQueue Receiving Failed");
            pthread_exit(0);
        }

		syslog (LOG_INFO, "Reader Got Accel X: %lf", reader_data.accel_x);
		syslog (LOG_INFO, "Reader Got Accel Y: %lf", reader_data.accel_y);
		syslog (LOG_INFO, "Reader Got Accel Z: %lf", reader_data.accel_z);
		syslog (LOG_INFO, "Reader Got Roll: %lf", reader_data.roll);
		syslog (LOG_INFO, "Reader Got Pitch: %lf", reader_data.pitch);
		syslog (LOG_INFO, "Reader Got Yaw: %lf", reader_data.yaw);
		syslog (LOG_INFO, "Reader Got Timestamp - Sec: %ld Nano Sec: %ld", reader_data.timestamp_local.tv_sec, reader_data.timestamp_local.tv_nsec);

		syslog (LOG_INFO, "<%.3fus>Reader Completed - Core(%d)", Time_Stamp(), sched_getcpu());

		i += 1;
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fus>Reader Exiting...", Time_Stamp());
	mq_close(custom_queue);
	mq_unlink(queue_name);
	pthread_exit(0);
}
