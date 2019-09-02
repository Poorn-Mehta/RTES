/*
*		File: Writer_Therad.c
*		Purpose: 
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#include "main.h"

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

void *Writer_Func(void *threadp)
{

    data_strct writer_data;

	// Queue setup
	mqd_t custom_queue;
	struct mq_attr custom_queue_attr;
//	struct mq_att tmp_attr;
	custom_queue_attr.mq_maxmsg = queue_size;
	custom_queue_attr.mq_msgsize = sizeof(writer_data);

	mq_unlink(queue_name);

	custom_queue = mq_open(queue_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &custom_queue_attr);
	if(custom_queue == -1)
	{
		syslog(LOG_ERR, "\nQueue opening error for Writer");
		pthread_exit(0);
	}

//	mq_getattr(custom_queue, &tmp_attr);
//	printf("\nSize should be %d but is %d", sizeof(writer_data), tmp_attr.mq_msgsize);

	int q_send_resp;

	uint8_t i = 0;

	while(i < 1)
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

		syslog (LOG_INFO, "Writer Wrote Accel X: %lf", writer_data.accel_x);
		syslog (LOG_INFO, "Writer Wrote Accel Y: %lf", writer_data.accel_y);
		syslog (LOG_INFO, "Writer Wrote Accel Z: %lf", writer_data.accel_z);
		syslog (LOG_INFO, "Writer Wrote Roll: %lf", writer_data.roll);
		syslog (LOG_INFO, "Writer Wrote Pitch: %lf", writer_data.pitch);
		syslog (LOG_INFO, "Writer Wrote Yaw: %lf", writer_data.yaw);
		syslog (LOG_INFO, "Writer Wrote Timestamp - Sec: %ld Nano Sec: %ld", writer_data.timestamp_local.tv_sec, writer_data.timestamp_local.tv_nsec);

		q_send_resp = mq_send(custom_queue,&writer_data,sizeof(data_strct),0);

        if(q_send_resp < 0)
        {
            syslog(LOG_ERR, "\nQueue sending error for Writer");
            printf("\nError Code: %d", q_send_resp);
            perror("\nQueue Sending Failed");
            pthread_exit(0);
        }

		syslog (LOG_INFO, "<%.3fus>Writer Completed - Core(%d)", Time_Stamp(), sched_getcpu());

		i += 1;
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fus>Writer Exiting...", Time_Stamp());
    mq_close(custom_queue);
	pthread_exit(0);
}
