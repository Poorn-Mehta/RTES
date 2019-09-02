/*
*		File: main.h
*		Purpose: The header file containing necessary headers, global defines, and function prototypes
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>

#define Thread1_ID      1
#define Thread2_ID      2
#define Thread3_ID      3
#define Thread4_ID      4

#define Starting_Value	50

void *threadsafe_reentrant(int thread_id);
