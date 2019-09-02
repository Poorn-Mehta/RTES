/*
*		File: Thread.c
*		Purpose: The source file containing threadsafe reentrant function, to demo the capabilities and utility of the same
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#include "main.h"

// Reentrant Thread Safe Function
void *threadsafe_reentrant(int thread_id)
{
    // Local variables
    uint8_t i = Starting_Value; // i is set to this value for each thread 
    uint8_t j;

    // Loop 10 times to display that there is no data corruption
    for(j = 0; j < 10; j ++)
    {
	// Switching based on thread id (the variable that has been passed to this function)
	// and performing different simple functions for each case
        switch(thread_id)
        {
            case    Thread1_ID:
                i += 1;
                break;
            case    Thread2_ID:
                i -= 1;
                break;
            case    Thread3_ID:
                i += 5;
                break;
            case    Thread4_ID:
                i -= 5;
                break;
            default:
                break;
        }

	// Printing outputs
	printf("Thread %d is Printing Value %d\n", thread_id, i);
    }

    // Terminating Thread
    pthread_exit(0);
}
