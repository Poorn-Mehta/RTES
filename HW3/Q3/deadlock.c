#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

#define NUM_THREADS 2
#define THREAD_1 1
#define THREAD_2 2

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


void *grabRsrcs(void *threadp)
{
   threadParams_t *threadParams = (threadParams_t *)threadp;
   int threadIdx = threadParams->threadIdx;


   if(threadIdx == THREAD_1)
   {
     printf("THREAD 1 grabbing resources\n");
     pthread_mutex_lock(&Lock_A);
     Counter_A++;
     if(!noWait) usleep(1000000);
     printf("THREAD 1 got A, trying for B\n");
     if(backoff == 1)
     {
         if(Counter_B != 0)
         {
            pthread_mutex_unlock(&Lock_A);
            do
            {
                random_sleep_A = rand() % 10000;
            }while((random_sleep_A == 0) || (random_sleep_A == random_sleep_B));
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
   else
   {
     printf("THREAD 2 grabbing resources\n");
     pthread_mutex_lock(&Lock_B);
     Counter_B++;
     if(!noWait) usleep(1000000);
     printf("THREAD 2 got B, trying for A\n");
     if(backoff == 1)
     {
         if(Counter_A != 0)
         {
            pthread_mutex_unlock(&Lock_B);
            do
            {
                random_sleep_B = rand() % 10000;
            }while((random_sleep_B == 0) || (random_sleep_B == random_sleep_A));
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
   pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
   int rc, safe=0;

   Counter_A=0, Counter_B=0, noWait=0;

   random_sleep_A = 0, random_sleep_B = 0, backoff = 0;

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
/*
   printf("Simple Test\n");
   printf("Locking A\n");
   pthread_mutex_lock(&Lock_A);
   printf("A Locked.. Unlocking A\n");
   pthread_mutex_unlock(&Lock_A);
   printf("Locking B\n");
   pthread_mutex_lock(&Lock_B);
   printf("B Locked.. Unlocking B\n");
   pthread_mutex_unlock(&Lock_B);
   printf("Test Completed\n");*/

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

   printf("Creating thread %d\n", THREAD_2);
   threadParams[THREAD_2 - 1].threadIdx=THREAD_2;
   rc = pthread_create(&threads[1], NULL, grabRsrcs, (void *)&threadParams[THREAD_2 - 1]);
   if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
   printf("Thread 2 spawned\n");

   printf("Counter_A=%d, Counter_B=%d\n", Counter_A, Counter_B);
   printf("will try to join CS threads unless they deadlock\n");

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
