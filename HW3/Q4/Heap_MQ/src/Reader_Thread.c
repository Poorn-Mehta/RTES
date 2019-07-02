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

// Following function implements Fib20 Thread
void *Reader_Func(void *threadp)
{

    char buffer[queue_len_bytes];
    void *buffptr;
    int prio;
    int id;

	// Queue setup
	mqd_t custom_queue;
	struct mq_attr custom_queue_attr;
	custom_queue_attr.mq_maxmsg = queue_size;
	custom_queue_attr.mq_msgsize = queue_len_bytes;

	custom_queue = mq_open(queue_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &custom_queue_attr);
	if(custom_queue == -1)
	{
		syslog(LOG_ERR, "\nQueue opening error for Reader");
		pthread_exit(0);
	}

	int q_recv_resp;

	uint8_t i = 0;

	while(i < 3)
	{

		syslog (LOG_INFO, "<%.3fus>Reader Started - Core(%d)", Time_Stamp(), sched_getcpu());

		q_recv_resp = mq_receive(custom_queue, &buffer[0], queue_len_bytes, &prio);

        if(q_recv_resp < 0)
        {
            syslog(LOG_ERR, "Queue receiving error for Reader");
            perror("\nQueue Receiving Failed");
            pthread_exit(0);
        }

        memcpy(&buffptr, buffer, sizeof(void *));
        memcpy((void *)&id, &(buffer[sizeof(void *)]), sizeof(int));

        syslog (LOG_INFO, "Receive: ptr msg 0x%X received with priority = %d, length = %d, id = %d", buffptr, prio, q_recv_resp, id);

        syslog (LOG_INFO, "Contents of ptr = %s", (char *)buffptr);

        free(buffptr);

        syslog (LOG_INFO, "heap space memory freed\n");

		syslog (LOG_INFO, "<%.3fus>Reader Completed - Core(%d)", Time_Stamp(), sched_getcpu());

		i += 1;

		sleep(1);
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fus>Reader Exiting...", Time_Stamp());
	mq_close(custom_queue);
	mq_unlink(queue_name);
	pthread_exit(0);
}
