/*
*		File: main.c
*		Purpose: The source file containing main()
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/


#include "main.h"

pthread_t threads[3];

int global_indexed[3] = {0};

int main()
{

    printf("\n >>>Program Start<<< \n\n");
    
    // Creating multiple threads
    pthread_create(&threads[0], NULL, thread1_func, NULL);   
    pthread_create(&threads[1], NULL, thread2_func, NULL);   
    pthread_create(&threads[2], NULL, thread3_func, NULL);   

    // Waiting for all threads to exit
    pthread_join(threads[0], 0);
    pthread_join(threads[1], 0);
    pthread_join(threads[2], 0);   

  
    printf("\n >>>Program End<<< \n\n");
  
    return 0;
}
