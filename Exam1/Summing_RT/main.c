// Platform: Jetson NANO

#define _GNU_SOURCE

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>

#include <syslog.h>
#include <inttypes.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define No_of_Cores	4

#define Sum_Thread_Const        100

#define Thread_0_99            0
#define Thread_100_199         1
#define Thread_200_299         2

#define Policy_Other        0
#define Policy_FIFO        1

float Time_Stamp(void);
void Set_Logger(char *logname, int level_upto);
uint8_t Realtime_Setup(uint8_t Bind_to_CPU);

// To record start time
struct timespec start_time = {0, 0};

// To terminate threads
uint8_t Terminate_Flag = 0;

// Thread handles
pthread_t summing_thread_handle[3];

// Thread attribute variable
pthread_attr_t Attr_All;

// Scheduler parameter
struct sched_param Attr_Sch;

// Realtime Priority variables for FIFO
uint8_t FIFO_Max_Prio = 99, FIFO_Min_Prio = 1;

pthread_mutex_t Mutex_Locker;

//info w_info, r_info;

// CPU Core Set
cpu_set_t CPU_Core;

uint32_t sum_threads[3] = {0};

typedef struct{
    int thread_id;
} thr_strct;

thr_strct thr_param_arr[3];

// Function to return relative timestamps
float Time_Stamp(void)
{
	struct timespec curr_time = {0, 0};

	// Record current time
	clock_gettime(CLOCK_REALTIME, &curr_time);
	float rval = 0;

	// Calculate Difference
	if(curr_time.tv_nsec >= start_time.tv_nsec)
		rval = ((curr_time.tv_sec - start_time.tv_sec) * 1000) + (((float)(curr_time.tv_nsec - start_time.tv_nsec)) / 1000000);
	else
		rval = ((curr_time.tv_sec - start_time.tv_sec - 1) * 1000) + (((float)(1000000000 - (start_time.tv_nsec - curr_time.tv_nsec))) / 1000000);

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


// Threadsafe Reentrant Function. Works the same way in both case - with or without FIFO
void *Summing_Thread(void* thread_para_ptr)
{
    // Get and store thread index (0. 1. 2) in local variable
    int thread_index = ((thr_strct*)thread_para_ptr)->thread_id;

    // Log the event
    syslog (LOG_INFO, "<%.3fms CustomEvent>Thread %d Starts", Time_Stamp(), thread_index);
    int i;

    // Sum and store in thread indexed global variable - this will be different for each thread
    for(i = (thread_index * Sum_Thread_Const); i < ((thread_index + 1) * Sum_Thread_Const); i ++)
    {
        sum_threads[thread_index] += i;
    }

    // Post Message and individual sum based on the thread index/thread id
    switch(thread_index)
    {
        case Thread_0_99:
        {
            syslog (LOG_INFO, "<%.3fms>Thread 0 Calculated 0 from 99, Result is: %d", Time_Stamp(), sum_threads[Thread_0_99]);
            break;
        }
        case Thread_100_199:
        {
            syslog (LOG_INFO, "<%.3fms>Thread 1 Calculated 100 from 199, Result is: %d", Time_Stamp(), sum_threads[Thread_100_199]);
            break;
        }
        case Thread_200_299:
        {
            syslog (LOG_INFO, "<%.3fms>Thread 2 Calculated 200 from 299, Result is: %d", Time_Stamp(), sum_threads[Thread_200_299]);
            break;
        }
        default:
        {
            break;
        }
    }

    // Log the exit event
    syslog (LOG_INFO, "<%.3fms CustomEvent>Thread %d Ends", Time_Stamp(), thread_index);
    pthread_exit(0);
}

int main (int argc, char *argv[])
{
    int sched_type;

	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

    // Setup logger
	Set_Logger("Ex1_Sum", LOG_DEBUG);

	syslog (LOG_INFO, ">>>>>>>>>> Program Start <<<<<<<<<<");

	// If command line argument is passed, and if that matches string FIFO then do a real time setup
	// Else leave the scheduler as it is
    if(argc < 2)
    {
        printf("\nNo Arguments Passed - Type FIFO as command line argument to set FIFO Scheduling\n");
        syslog(LOG_INFO, "No Arguments Passed - Type FIFO as command line argument to set FIFO Scheduling");
        syslog(LOG_INFO, "Starting Program with OTHER Scheduling Policy");
        sched_type = Policy_Other;
    }

    else if(argc == 2)
    {
        if(strncmp("FIFO", argv[1], 4) == 0)
        {
            printf("\nStarting Program with FIFO Scheduling Policy\n");
            syslog(LOG_INFO, "Starting Program with FIFO Scheduling Policy");
            sched_type = Policy_FIFO;
        }
        else
        {
            syslog(LOG_INFO, "Argument Not Recognized");
            syslog(LOG_INFO, "Starting Program with OTHER Scheduling Policy");
            sched_type = Policy_Other;
        }
    }

    else
    {
        syslog(LOG_INFO, "Too many arguments");
        syslog(LOG_INFO, "Starting Program with OTHER Scheduling Policy");
        sched_type = Policy_Other;
    }

	printf("\nThis Program uses Syslog instead of printf\n");
	printf("\nExecute following to see the output:\n\ncd /var/log && grep Ex1_Sum syslog\n");

	// Check whether FIFO should be used or not
	// If it is to be used, then call the function to do the same
    if(sched_type == Policy_FIFO)
    {
     	// Setup the system for realtime execution, use CPU3
        if(Realtime_Setup(3) != 0)
        {
            exit(-1);
        }
    }

	int i;

	// If it is FIFO scheduler, then set priorities of each threads, and then pass the attribute while creating a thread
	if(sched_type == Policy_FIFO)
	{
		for(i = 0; i < 3; i ++)
        {
            thr_param_arr[i].thread_id = i;
            Attr_Sch.sched_priority = FIFO_Max_Prio - (i + 1);
            pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
            pthread_create(&summing_thread_handle[i], &Attr_All, Summing_Thread, (void *)&thr_param_arr[i]);
        }
	}

	// Simply create threads without passing any attributes in case of the OTHER scheduler
	else
	{
        for(i = 0; i < 3; i ++)
        {
            thr_param_arr[i].thread_id = i;
            pthread_create(&summing_thread_handle[i], (void *)0, Summing_Thread, (void *)&thr_param_arr[i]);
        }
	}

	// Wait for all threads to terminate
	for(i = 0; i < 3; i ++)
	{
       pthread_join(summing_thread_handle[i], 0);
	}

	// Log closing events
	syslog (LOG_INFO, "<%.3fms>Total Sum is: %d", Time_Stamp(), (sum_threads[Thread_0_99] + sum_threads[Thread_100_199] + sum_threads[Thread_200_299]));

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Close the log
	closelog();

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);


	// Exit the program
	exit(0);
}
