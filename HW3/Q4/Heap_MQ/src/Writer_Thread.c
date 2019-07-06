/*
*		File: Writer_Thread.c
*		Purpose: The source file containing implementation of writer thread for demonstrating IPC using heap allocated shared memory and Posix Queues
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

	// Following section writes data (64 ASCII Characters) to a large buffer
	char imagebuff[4096];
	uint16_t i, j;
	char pixel = 'A';

	for(i = 0; i < 4096; i += 64)
	{
	pixel = 'A';
	for(j = i; j < (i + 64); j ++)
	{
	    imagebuff[j] = (char)pixel++;
	}
	//        imagebuff[j-1] = '\n';
	}
	imagebuff[4095] = '\0';
	imagebuff[63] = '\0';

	syslog(LOG_INFO, "Wrote Image Buffer: %s", &imagebuff[0]);

	// This buffer will hold the data that will be actually transferred using Posix Queue
	char buffer[queue_len_bytes];

	// Heap pointer that points to start of the data being shared
	void *buffptr;

	uint8_t prio = 30;
	int id = 999;

	// Queue setup
	mqd_t custom_queue;
	struct mq_attr custom_queue_attr;
//	struct mq_att tmp_attr;
	custom_queue_attr.mq_maxmsg = queue_size;
	custom_queue_attr.mq_msgsize = queue_len_bytes;

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

		// Dynamically allocated memory in heap for the data to be shared
		buffptr = (void *)malloc(sizeof(imagebuff));

		// Copy data from static array to this allocated memory region in heap
		strcpy(buffptr, &imagebuff[0]);

		// Ensure that data has been copied well
		syslog (LOG_INFO, "Message to send = %s", (char *)buffptr);

		// To visualize how many bytes we are going to send to share this large structure
		syslog (LOG_INFO, "Sending %ld bytes", sizeof(buffptr));

		// Copy pointer address to the array that will be sent over queue
		memcpy(buffer, &buffptr, sizeof(void *));

		// Copy id in later bytes of the same array
		memcpy(&(buffer[sizeof(void *)]), (void *)&id, sizeof(int));

		// Send message using posix queue
		q_send_resp = mq_send(custom_queue, &buffer[0], queue_len_bytes, prio);

		// Error Handling
		if(q_send_resp < 0)
		{
		    syslog(LOG_ERR, "nQueue sending error for Writer");
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
