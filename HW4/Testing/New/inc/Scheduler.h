/*
*		File: Scheduler.h
*		Purpose: The header file containing useful defines and function prototypes for related source file
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*/

#ifndef	__SCHEDULER_H__
#define __SCHEDULER_H__

// The amount of delay to simlply wait for two other real time threads to exit
#define Post_Exit_Wait_ms	(uint32_t)100

// Function prototype
void *Scheduler_Func(void *para_t);

#endif
