/*
*		File: deadlock.c
*		Purpose: The source file to demonstrate deadlock and various ways to resolve the same
*		Originally Written By: Sam Siewert (http://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/code/example-sync/deadlock.c)
*		Updated and Modified By: Poorn Mehta
*		Last Modified: 7/5/2019
*
*		Modifications: I've added random backoff method to resolve the deadlock. It can be verified by entering 'backoff' as command line argument
*			       There was a bug which was referring to third index of threads[] array - which was causing some undefined behavior. I have fixed that as well.
*
*/

#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

#define NUM_THREADS 2
#define THREAD_1 1
#define THREAD_2 2

// Global Variables

typedef struct
{
    int threadIdx;
} threadParams_t;


pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];

struct sched_param nrt_param;

pthread_mutex_t Lock_A, Lock_B;

volatile int Counter_A=0, Counter_B=0, noWait=0;

unsigned char random_sleep_A, random_sleep_B, backoff;

// Function for both threads (it switches operation based on thread id)
void *grabRsrcs(void *threadp)
{
   // Getting thread ID
   threadParams_t *threadParams = (threadParams_t *)threadp;
   int threadIdx = threadParams->threadIdx;

   // Implementation for thread 1
   if(threadIdx == THREAD_1)
   {
     // Grabbing Lock_A mutex and then sleeping for 100ms to ensure other thread has grabbed mutex as well
     printf("THREAD 1 grabbing resources\n");
     pthread_mutex_lock(&Lock_A);
     Counter_A++;
     if(!noWait) usleep(1000000);
     printf("THREAD 1 got A, trying for B\n");

     // If random backoff is enabled, then implement that
     if(backoff == 1)
     {
	 // Check if other resource is already acquired or not
	 // If it is acquired, release Lock_A mutex, sleep for random time, and then lock both quickly
         if(Counter_B != 0)
         {
            pthread_mutex_unlock(&Lock_A);
            do
            {
                random_sleep_A = rand() % 10000;
            }while((random_sleep_A == 0) || (random_sleep_A == random_sleep_B)); // Ensuring that this sleep won't result in another deadlock
            usleep(random_sleep_A);
            pthread_mutex_lock(&Lock_A);
         }
     }
     pthread_mutex_lock(&Lock_B);
     Counter_B++;
     printf("THREAD 1 got A and B\n");
     pthread_mutex_unlock(&Lock_B);
     pthread_mutex_unlock(&Lock_A);
     printf("THREAD 1 done\n");
   }
   
   // Implementation for thread 2
   else
   {
     // Grabbing Lock_B mutex and then sleeping for 100ms to ensure other thread has grabbed mutex as well
     printf("THREAD 2 grabbing resources\n");
     pthread_mutex_lock(&Lock_B);
     Counter_B++;
     if(!noWait) usleep(1000000);
     printf("THREAD 2 got B, trying for A\n");

     // If random backoff is enabled, then implement that
     if(backoff == 1)
     {
	 // Check if other resource is already acquired or not
	 // If it is acquired, release Lock_B mutex, sleep for random time, and then lock both quickly
         if(Counter_A != 0)
         {
            pthread_mutex_unlock(&Lock_B);
            do
            {
                random_sleep_B = rand() % 10000;
            }while((random_sleep_B == 0) || (random_sleep_B == random_sleep_A)); // Ensuring that this sleep won't result in another deadlock
            usleep(random_sleep_B);
            pthread_mutex_lock(&Lock_B);
         }
     }
     pthread_mutex_lock(&Lock_A);
     Counter_A++;
     printf("THREAD 2 got B and A\n");
     pthread_mutex_unlock(&Lock_A);
     pthread_mutex_unlock(&Lock_B);
     printf("THREAD 2 done\n");
   }

   // Terminating
   pthread_exit(NULL);
}

int main (int argc, char *argv[])
{

   // Initialization

   int rc, safe=0;

   Counter_A=0, Counter_B=0, noWait=0;

   random_sleep_A = 0, random_sleep_B = 0, backoff = 0;

   // Check for valid command line arguments

   if(argc < 2)
   {
     printf("Will set up unsafe deadlock scenario\n");
   }
   else if(argc == 2)
   {
     if(strncmp("safe", argv[1], 4) == 0)
       safe=1;
     else if(strncmp("race", argv[1], 4) == 0)
       noWait=1;
     else if(strncmp("backoff", argv[1], 7) == 0)
     {
        printf("Using Random Backoff Scheme\n");
        backoff = 1;
     }
     else
       printf("Will set up unsafe deadlock scenario\n");
   }
   else
   {
     printf("Usage: deadlock [safe|race|unsafe]\n");
   }

   // Set default protocol for mutex
   pthread_mutex_init(&Lock_A, NULL);
   pthread_mutex_init(&Lock_B, NULL);

   // Creating thread 1
   printf("Creating thread %d\n", THREAD_1);
   threadParams[THREAD_1 - 1].threadIdx=THREAD_1;
   rc = pthread_create(&threads[0], NULL, grabRsrcs, (void *)&threadParams[THREAD_1 - 1]);
   if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
   printf("Thread 1 spawned\n");

   if(safe) // Make sure Thread 1 finishes with both resources first
   {
     if(pthread_join(threads[0], NULL) == 0)
       printf("Thread 1: %x done\n", (unsigned int)threads[0]);
     else
       perror("Thread 1");
   }

   // Creating thread 1
   printf("Creating thread %d\n", THREAD_2);
   threadParams[THREAD_2 - 1].threadIdx=THREAD_2;
   rc = pthread_create(&threads[1], NULL, grabRsrcs, (void *)&threadParams[THREAD_2 - 1]);
   if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
   printf("Thread 2 spawned\n");

   printf("Counter_A=%d, Counter_B=%d\n", Counter_A, Counter_B);
   printf("will try to join CS threads unless they deadlock\n");

   // Waiting for threads to complete
   if(!safe)
   {
     if(pthread_join(threads[0], NULL) == 0)
       printf("Thread 1: %x done\n", (unsigned int)threads[0]);
     else
       perror("Thread 1");
   }

   if(pthread_join(threads[1], NULL) == 0)
     printf("Thread 2: %x done\n", (unsigned int)threads[1]);
   else
     perror("Thread 2");

   // Destroying Mutexes
   if(pthread_mutex_destroy(&Lock_A) != 0)
     perror("mutex A destroy");
   else
     printf("mutex A destroyed\n");

   if(pthread_mutex_destroy(&Lock_B) != 0)
     perror("mutex B destroy");
   else
     printf("mutex B destroyed\n");

   printf("All done\n");

   exit(0);
}
