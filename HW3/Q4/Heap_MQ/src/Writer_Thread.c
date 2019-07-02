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

//extern uint8_t queue_len_bytes;

// Following function implements Fib10 Thread
void *Writer_Func(void *threadp)
{

    char imagebuff[4096];
    uint16_t i, j;
    char pixel = 'A';

    for(i = 0; i < (0x0001 << 12); i += (0x0001 << 6))
    {
        pixel = 'A';
        for(j = i; j < (i + 64); j ++)
        {
            imagebuff[j] = (char)pixel++;
        }
        imagebuff[j-1] = '\n';
    }
    imagebuff[4095] = '\0';
    imagebuff[63] = '\0';

    syslog(LOG_INFO, "Wrote Image Buffer: \n%s", &imagebuff[0]);

    char buffer[queue_len_bytes];
    void *buffptr;
    uint8_t prio = 30;
    int id = 999;

	// Queue setup
	mqd_t custom_queue;
	struct mq_attr custom_queue_attr;
//	struct mq_att tmp_attr;
	custom_queue_attr.mq_maxmsg = queue_size;
	custom_queue_attr.mq_msgsize = queue_len_bytes;

	mq_unlink(queue_name);

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

        buffptr = (void *)malloc(sizeof(imagebuff));
        strcpy(buffptr, &imagebuff[0]);
        syslog (LOG_INFO, "Message to send = %s\n", (char *)buffptr);

        syslog (LOG_INFO, "Sending %ld bytes\n", sizeof(buffptr));

        memcpy(buffer, &buffptr, sizeof(void *));
        memcpy(&(buffer[sizeof(void *)]), (void *)&id, sizeof(int));

		q_send_resp = mq_send(custom_queue, &buffer[0], queue_len_bytes, prio);

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
