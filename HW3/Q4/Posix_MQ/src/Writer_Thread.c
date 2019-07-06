/*
*		File: Writer_Thread.c
*		Purpose: The source file containing implementation of writer thread for demonstrating IPC Posix Queues
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#include "main.h"

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

//extern uint8_t queue_len_bytes;

// Following function implements Writer Thread
void *Writer_Func(void *threadp)
{

    uint8_t prio = 30;

	// Queue setup
	mqd_t custom_queue;
	struct mq_attr custom_queue_attr;
//	struct mq_att tmp_attr;
	custom_queue_attr.mq_maxmsg = queue_size;
	custom_queue_attr.mq_msgsize = MAX_MSG_SIZE;

	// Unlinking just for safety/cleanup from previous code
	mq_unlink(queue_name);

	// Open queue with write only access
	custom_queue = mq_open(queue_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &custom_queue_attr);
	if(custom_queue == -1)
	{
		syslog(LOG_ERR, "\nQueue opening error for Writer");
		pthread_exit(0);
	}

	int q_send_resp;

	uint8_t k = 0;

	while(k < 3)
	{

		syslog (LOG_INFO, "<%.3fus>Writer Started - Core(%d)", Time_Stamp(), sched_getcpu());

		// Print out the message which is going be sent through queue
		syslog (LOG_INFO, "Message to send = %s\n", (char *)&canned_msg[0]);

		syslog (LOG_INFO, "Sending %ld bytes\n", sizeof(canned_msg));

		// Send the whole message through the POSIX Queue
		q_send_resp = mq_send(custom_queue, canned_msg, sizeof(canned_msg), prio);

		// Error Handling
		if(q_send_resp < 0)
		{
		    syslog(LOG_ERR, "\nQueue sending error for Writer");
		    perror("\nQueue Sending Failed");
		    pthread_exit(0);
		}

		syslog (LOG_INFO, "<%.3fus>Writer Completed - Core(%d)", Time_Stamp(), sched_getcpu());

		k += 1;

		sleep(1);
	}

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.3fus>Writer Exiting...", Time_Stamp());	
	mq_close(custom_queue);
	pthread_exit(0);
}
