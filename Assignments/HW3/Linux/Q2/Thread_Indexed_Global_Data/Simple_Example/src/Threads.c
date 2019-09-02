/*
*		File: Threads.c
*		Purpose: The source file containing necessary functions to demonstrate the utility and method to implement globally indexed shared data
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#include "main.h"

// Shared global array of integers
extern int global_indexed[];

// Thread 1 function
void *thread1_func(void *tptr)
{
    // Constant pointer to integer. This prevents this thread from moving through the globally indexed array for write operation.
    // It can only update the value it points to. But never the memory location itself. Use it as a 'write-only' object.
    int *const t1_global = &global_indexed[0];	

    // Pointer to constant integer. This pointer can be moved around in array, but only for having read access.
    // Use it as a 'read-only' object.
    const int *other_global = &global_indexed[0];
    
    // Printing all global variables, updating own, and sleeping to see the effect

    printf("<Thread 1> Own Data: %d Thread 2 Data: %d Thread 3 Data: %d\n", *other_global, *(other_global+1), *(other_global+2));
    *t1_global = 1;
    
    sleep(1);
    
    printf("<Thread 1> Own Data: %d Thread 2 Data: %d Thread 3 Data: %d\n", *other_global, *(other_global+1), *(other_global+2));
    *t1_global += 10;
    
    sleep(1);
    
    printf("<Thread 1> Own Data: %d Thread 2 Data: %d Thread 3 Data: %d\n", *other_global, *(other_global+1), *(other_global+2));
 
    // Exiting Thread   
    pthread_exit(0);
}

// Thread 2 function
void *thread2_func(void *tptr)
{
    // Constant pointer to integer. This prevents this thread from moving through the globally indexed array for write operation.
    // It can only update the value it points to. But never the memory location itself. Use it as a 'write-only' object.
    int *const t2_global = &global_indexed[1];

    // Pointer to constant integer. This pointer can be moved around in array, but only for having read access.
    // Use it as a 'read-only' object.
    const int *other_global = &global_indexed[0];
    
    // Printing all global variables, updating own, and sleeping to see the effect

    printf("<Thread 2> Own Data: %d Thread 1 Data: %d Thread 3 Data: %d\n", *(other_global+1), *other_global, *(other_global+2));
    *t2_global = 2;
    
    sleep(1);
    
    printf("<Thread 2> Own Data: %d Thread 1 Data: %d Thread 3 Data: %d\n", *(other_global+1), *other_global, *(other_global+2));
    *t2_global += 10;
    
    sleep(1);
    
    printf("<Thread 2> Own Data: %d Thread 1 Data: %d Thread 3 Data: %d\n", *(other_global+1), *other_global, *(other_global+2));

    // Exiting Thread       
    pthread_exit(0);
}

// Thread 3 function
void *thread3_func(void *tptr)
{
    // Constant pointer to integer. This prevents this thread from moving through the globally indexed array for write operation.
    // It can only update the value it points to. But never the memory location itself. Use it as a 'write-only' object.
    int *const t3_global = &global_indexed[2];

    // Pointer to constant integer. This pointer can be moved around in array, but only for having read access.
    // Use it as a 'read-only' object.
    const int *other_global = &global_indexed[0];
    
    // Printing all global variables, updating own, and sleeping to see the effect

    printf("<Thread 3> Own Data: %d Thread 1 Data: %d Thread 2 Data: %d\n", *(other_global+2), *other_global, *(other_global+1));
    *t3_global = 3;
    
    sleep(1);
    
    printf("<Thread 3> Own Data: %d Thread 1 Data: %d Thread 2 Data: %d\n", *(other_global+2), *other_global, *(other_global+1));
    *t3_global += 10;
    
    sleep(1);
    
    printf("<Thread 3> Own Data: %d Thread 1 Data: %d Thread 2 Data: %d\n", *(other_global+2), *other_global, *(other_global+1));
    
    // Exiting Thread
    pthread_exit(0);
}
