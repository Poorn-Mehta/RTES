// Platform: Jetson NANO

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#include <syslog.h>

#include <semaphore.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <netdb.h>
#include <malloc.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/utsname.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define No_of_Cores	4

#define Policy_Other        0
#define Policy_FIFO        1

#define Mode_sec	(uint8_t)1
#define Mode_ms		(uint8_t)2
#define Mode_us		(uint8_t)3

#define s_to_ns			(uint32_t)1000000000
#define s_to_us			(uint32_t)1000000
#define s_to_ms			(uint32_t)1000
#define ms_to_ns		(uint32_t)1000000
#define ms_to_us		(uint32_t)1000
#define us_to_ns		(uint32_t)1000
#define ns_to_us		(float)0.001
#define ns_to_ms		(float)0.000001
#define ns_to_s			(float)0.000000001

#define w1_q_name 		"/work1_q"
#define w2_q_name 		"/work2_q"

float Time_Stamp(uint8_t mode);
void Set_Logger(char *logname, int level_upto);
uint8_t Realtime_Setup(uint8_t Bind_to_CPU);

typedef struct{
	uint32_t number;
} queue_strct;

sem_t W1_Sem, W2_Sem;

// To record start time
struct timespec start_time = {0, 0};

// To terminate threads
uint8_t Terminate_Flag = 0;

// Thread handles
pthread_t S1_Handle, W1_Handle, W2_Handle;

// Thread attribute variable
pthread_attr_t Attr_All;

// Scheduler parameter
struct sched_param Attr_Sch;

// Realtime Priority variables for FIFO
uint8_t FIFO_Max_Prio = 99, FIFO_Min_Prio = 1;

// CPU Core Set
cpu_set_t CPU_Core;

// Function to return relative timestamps
float Time_Stamp(uint8_t mode)
{
	struct timespec curr_time = {0, 0};
	uint32_t multiplier;
	float inv_multiplier;
	switch(mode)
	{
		case Mode_sec:
		{
			multiplier = 1;
			inv_multiplier = ns_to_s;
			break;
		}

		case Mode_ms:
		{
			multiplier = s_to_ms;
			inv_multiplier = ns_to_ms;
			break;
		}

		case Mode_us:
		{
			multiplier = s_to_us;
			inv_multiplier = ns_to_us;
			break;
		}

		default:
		{
			multiplier = 1;
			inv_multiplier = ns_to_s;
			break;
		}
	}


	// Record current time
	clock_gettime(CLOCK_REALTIME, &curr_time);
	float rval = 0;

	// Calculate Difference
	if(curr_time.tv_nsec >= start_time.tv_nsec)
	{
        	rval = ((curr_time.tv_sec - start_time.tv_sec) * multiplier) + (((float)(curr_time.tv_nsec - start_time.tv_nsec)) * inv_multiplier);
	}

	else
	{
        	rval = ((curr_time.tv_sec - start_time.tv_sec - 1) * multiplier) + (((float)(s_to_ns - (start_time.tv_nsec - curr_time.tv_nsec))) * inv_multiplier);
	}

	return (rval);
}

// Setup Logger as needed
void Set_Logger(char *logname, int level_upto)
{
	// Logmask - the lowest (least important) Log Level to be written
	setlogmask(LOG_UPTO(level_upto));

	// Open log with prefix name
	openlog(logname, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
}

// Setting up system for realtime operation
uint8_t Realtime_Setup(uint8_t Bind_to_CPU)
{
	int sched;

	// Initializing Attribute which will be passed to all pthreads
	if(pthread_attr_init(&Attr_All) != 0)
	{
		syslog(LOG_ERR, "Failed to Initialize Attr_All");
		return 1;
	}

	// Preventing pthreads to inherit scheduler, and tell them explicity to use given scheduler (through another API)
	if(pthread_attr_setinheritsched(&Attr_All, PTHREAD_EXPLICIT_SCHED) != 0)
	{
		syslog(LOG_ERR, "Failed to Set Schedule Inheritance");
		return 1;
	}

	// Set scheduling policy to FIFO for all threads
	if(pthread_attr_setschedpolicy(&Attr_All, SCHED_FIFO) != 0)
	{
		syslog(LOG_ERR, "Failed to Set Scheduling Policy");
		return 1;
	}

	// Bind all threads to a single CPU core
	if(Bind_to_CPU < No_of_Cores)
	{
		// First, clearing variable
		CPU_ZERO(&CPU_Core);

		// Adding core-<Bind_to_CPU> in the variable
		CPU_SET(Bind_to_CPU, &CPU_Core);

		// Executing API to set the CPU
		if(pthread_attr_setaffinity_np(&Attr_All, sizeof(cpu_set_t), &CPU_Core) != 0)
		{
			syslog(LOG_ERR, "Failed to Set CPU Core");
			return 1;
		}
	}

	// Getting Max and Min realtime priorities for FIFO
	FIFO_Max_Prio = sched_get_priority_max(SCHED_FIFO);
	FIFO_Min_Prio = sched_get_priority_min(SCHED_FIFO);

	// Setting scheduler for the program (by setting parent thread to FIFO) (I am not 100% sure about this though)
	Attr_Sch.sched_priority = FIFO_Max_Prio;
	if(sched_setscheduler(getpid(), SCHED_FIFO, &Attr_Sch) != 0)
	{
		perror("sched_setscheduler");
		syslog(LOG_ERR, "Failed to Set FIFO Scheduler");
		return 1;
	}

	// Verify that Scheduler is set to FIFO
	sched = sched_getscheduler(getpid());
	if(sched == SCHED_FIFO)		syslog (LOG_INFO, "Scheduling set to FIFO");
	else
	{
		syslog(LOG_ERR, "Scheduling is not FIFO");
		return 1;
	}

	return 0;

}

void *S1_Thr(void* thread_para_ptr)
{
	// Queue setup
	mqd_t w1_q;
	queue_strct w1_strct;
	int q_send_resp;
	uint32_t sch_count = 1;

	struct mq_attr w1_queue_attr;
	w1_queue_attr.mq_maxmsg = 100;
	w1_queue_attr.mq_msgsize = sizeof(queue_strct);

	// w1_q write only
	w1_q = mq_open(w1_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &w1_queue_attr);

	// Error handling
	if(w1_q == -1)
	{
		syslog(LOG_ERR, "<%.6fms>!!S1_Thr!! W1 Queue opening error for Writing", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	w1_strct.number = 1;

	syslog (LOG_INFO, "<%.6fms>!!S1_Thr!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	// Total of 200 numbers, starting from 1
	while(w1_strct.number <= 200)
	{

		syslog (LOG_INFO, "<%.6fms>!!S1_Thr!! S1 Launched", Time_Stamp(Mode_ms));

		// Send integers, one by one
		q_send_resp = mq_send(w1_q, (const char *)&w1_strct, sizeof(queue_strct),0);

		// Error handling
		if(q_send_resp < 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!S1_Thr!! W1 Queue sending Error", Time_Stamp(Mode_ms));
			pthread_exit(0);
		}

		// For the frequency of W1
		if((sch_count % 2) == 0)
		{
			sem_post(&W1_Sem);
		}

		// For the frequency of W2
		if((sch_count % 20) == 0)
		{	
			sch_count = 0;
			sem_post(&W2_Sem);
		}

		sch_count += 1;

		syslog (LOG_INFO, "<%.6fms>!!S1_Thr!! S1 Completed... Last Queued: %d", Time_Stamp(Mode_ms), w1_strct.number);

		// Incrementing number to send to W1
		w1_strct.number += 1;

		// Sleep for 50ms to get 20Hz frequency
		usleep(50000);

	}
	
	// End
	syslog (LOG_INFO, "<%.6fms>!!S1_Thr!! Thread Exiting...", Time_Stamp(Mode_ms));

	pthread_exit(0);
}

void *W1_Thr(void* thread_para_ptr)
{
	// Queue setup
	mqd_t w1_q;
	queue_strct w1_strct;

	struct mq_attr w1_queue_attr;
	w1_queue_attr.mq_maxmsg = 100;
	w1_queue_attr.mq_msgsize = sizeof(queue_strct);

	// w1_q readonly
	w1_q = mq_open(w1_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &w1_queue_attr);

	// Error handling
	if(w1_q == -1)
	{
		syslog(LOG_ERR, "<%.6fms>!!W1_Thr!! W1 Queue opening error for Reading", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	mqd_t w2_q;
	queue_strct w2_strct;

	struct mq_attr w2_queue_attr;
	w2_queue_attr.mq_maxmsg = 100;
	w2_queue_attr.mq_msgsize = sizeof(queue_strct);

	// w2_q writeonly
	w2_q = mq_open(w2_q_name, O_CREAT | O_WRONLY | O_CLOEXEC, 0666, &w2_queue_attr);

	// Error handling
	if(w2_q == -1)
	{
		syslog(LOG_ERR, "<%.6fms>!!W1_Thr!! W2 Queue opening error for Writing", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	uint32_t count = 1, local_num = 0;
	int q_recv_resp, q_send_resp;

	syslog (LOG_INFO, "<%.6fms>!!W1_Thr!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	// run total of 100 times
	while(count <= 100)
	{
		sem_wait(&W1_Sem);

		syslog (LOG_INFO, "<%.6fms>!!W1_Thr!! W1 Launched", Time_Stamp(Mode_ms));

		q_recv_resp = mq_receive(w1_q, (char *)&w1_strct, sizeof(queue_strct), 0);
		if(q_recv_resp == sizeof(queue_strct))
		{
			// locally store the first dequeued number
			local_num = w1_strct.number;

			q_recv_resp = mq_receive(w1_q, (char *)&w1_strct, sizeof(queue_strct), 0);
			if(q_recv_resp == sizeof(queue_strct))
			{
				// sum first two numbers to send to w2_q
				w2_strct.number = local_num + w1_strct.number;
				count += 1;

				q_send_resp = mq_send(w2_q, (const char *)&w2_strct, sizeof(queue_strct),0);

				// Error handling
				if(q_send_resp < 0)
				{
					syslog(LOG_ERR, "<%.6fms>!!W1_Thr!! W2 Queue sending Error", Time_Stamp(Mode_ms));
					pthread_exit(0);
				}
			}

			// Error handling
			else
			{
				syslog(LOG_ERR, "<%.6fms>!!W1_Thr!! W1 Queue receiving Error", Time_Stamp(Mode_ms));
				pthread_exit(0);
			}
		}

		// Error handling
		else
		{
			syslog(LOG_ERR, "<%.6fms>!!W1_Thr!! W1 Queue receiving Error", Time_Stamp(Mode_ms));
			pthread_exit(0);
		}

		syslog (LOG_INFO, "<%.6fms>!!W1_Thr!! W1 Completed... Last Sum: %d", Time_Stamp(Mode_ms), w2_strct.number);

	}
	
	// End
	syslog (LOG_INFO, "<%.6fms>!!W1_Thr!! Thread Exiting...", Time_Stamp(Mode_ms));

	pthread_exit(0);
}

void *W2_Thr(void* thread_para_ptr)
{
	// Queue setup
	static mqd_t w2_q;
	static queue_strct w2_strct;

	struct mq_attr w2_queue_attr;
	w2_queue_attr.mq_maxmsg = 100;
	w2_queue_attr.mq_msgsize = sizeof(queue_strct);

	// Readonly
	w2_q = mq_open(w2_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &w2_queue_attr);

	// Error handling
	if(w2_q == -1)
	{
		syslog(LOG_ERR, "<%.6fms>!!W2_Thr!! W2 Queue opening error for Reading", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	syslog (LOG_INFO, "<%.6fms>!!W2_Thr!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	uint32_t count = 1, sum = 0, o_count = 1;
	int q_recv_resp;

	// o_count is used to run this summing function total of 100 times
	while(o_count <= 10)
	{
		// Wait for scheduler
		sem_wait(&W2_Sem);

		count = 1;

		// count represents the number of times the message has been dequeued from the w2_q
		while(count <= 10)
		{

			syslog (LOG_INFO, "<%.6fms>!!W2_Thr!! W2 Launched", Time_Stamp(Mode_ms));

			// Dequeue and sum up the received number (which is sum of 2 numbers)
			q_recv_resp = mq_receive(w2_q, (char *)&w2_strct, sizeof(queue_strct), 0);
			if(q_recv_resp == sizeof(queue_strct))
			{
				sum += w2_strct.number;
				count += 1;
			}

			// Error handling
			else
			{
				syslog(LOG_ERR, "<%.6fms>!!W2_Thr!! W2 Queue receiving Error", Time_Stamp(Mode_ms));
				pthread_exit(0);
			}
			
			syslog (LOG_INFO, "<%.6fms>!!W2_Thr!! W2 Completed... Overall Sum: %d", Time_Stamp(Mode_ms), sum);		

		}

		o_count += 1;
	}

	// End
	syslog (LOG_INFO, "<%.6fms>!!W2_Thr!! Final Sum: %d Expected: 20100", Time_Stamp(Mode_ms), sum);
	
	syslog (LOG_INFO, "<%.6fms>!!W2_Thr!! Thread Exiting...", Time_Stamp(Mode_ms));

	pthread_exit(0);

}

int main (int argc, char *argv[])
{

	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

    	// Setup logger
	Set_Logger("Ex2_Q1", LOG_DEBUG);

	syslog (LOG_INFO, ">>>>>>>>>> Program Start <<<<<<<<<<");

	printf("\nThis Program uses Syslog instead of printf\n");
	printf("\nExecute following to see the output:\n\ncd /var/log && grep -a Ex2_Q1 syslog\n");

     	// Setup the system for realtime execution, use CPU3
        if(Realtime_Setup(3) != 0)
        {
            exit(-1);
        }

	// Initialize Semaphores
	if(sem_init(&W1_Sem, 0, 0) != 0)
	{
	    syslog(LOG_ERR, "Failed to initialize W1_Sem semaphore");
	    exit(-1);
	}

	if(sem_init(&W2_Sem, 0, 0) != 0)
	{
	    syslog(LOG_ERR, "Failed to initialize W2_Sem semaphore");
	    exit(-1);
	}

	// Create all threads
	Attr_Sch.sched_priority = FIFO_Max_Prio - 2;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&W1_Handle, &Attr_All, W1_Thr, (void *)0);

	Attr_Sch.sched_priority = FIFO_Max_Prio - 3;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&W2_Handle, &Attr_All, W2_Thr, (void *)0);

	// Delay creation of scheduler so that other threads have enough time to complete setup
	usleep(50000);

	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&S1_Handle, &Attr_All, S1_Thr, (void *)0);

	// Wait for all threads to exit
	pthread_join(S1_Handle, 0);
	pthread_join(W1_Handle, 0);
	pthread_join(W2_Handle, 0);

	// Destroy semaphores
	sem_destroy(&W1_Sem);
	sem_destroy(&W2_Sem);

	// Log ending of the program
	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Close the log
	closelog();

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);


	// Exit the program
	exit(0);
}
