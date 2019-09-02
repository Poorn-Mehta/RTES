/*
*		File: main.c
*		Purpose: The source file containing main()
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#include "main.h"

// Thread ID
pthread_t threads[4];

int main(void)
{
    int i;
    
    printf("\n >>>Program Start<<< \n\n");

    // Creating multiple threads
    for(i = 0; i < 4; i ++)
    {
        pthread_create(&threads[i], 0, threadsafe_reentrant, i + 1);   
    }
    
    // Waiting for all threads to exit
    for(i = 0; i < 4; i ++)
    {
        pthread_join(threads[i], 0);
    }
    
    printf("\n >>>Program End<<< \n\n");

    return 0;
}
