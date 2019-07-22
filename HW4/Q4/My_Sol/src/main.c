/*
*		File: main.c
*		Purpose: The source file containing main() and global variables for Question 5 of Homework 3
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*		Linux Man pages has been referred whenever necessary. Also, I have referred to my own previous work (https://github.com/Poorn-Mehta/AESD) at some points.
*/

#include "main.h"

//#include <mqueue.h>
//#include <fcntl.h>
//#include <sys/stat.h>

// To record start time
struct timespec start_time = {0, 0};

// Thread handles
pthread_t Scheduler_Thread, Logger_Thread, Cam_Socket_Thread, Cam_RGB_Thread, Cam_Monitor_Thread, Cam_Compress_Thread, Cam_Grey_Thread;

// Thread attribute variable
pthread_attr_t Attr_All;

// Scheduler parameter
struct sched_param Attr_Sch;

// Realtime Priority variables for FIFO
uint8_t FIFO_Max_Prio = 99, FIFO_Min_Prio = 1;
//info w_info, r_info;

// CPU Core Set
cpu_set_t CPU_Core;

// Format is used by a number of functions, so made as a file global
// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-format
struct v4l2_format fmt;

char *dev_name;
//io_options io = IO_METHOD_USERPTR;
int fd = -1;
frame_p_buffer *frame_p;
uint32_t n_buffers;
int out_buf;
int force_format = 1;
int frame_count = 30;
uint8_t Max_Throughput = 0;

int main (int argc, char *argv[])
{
	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

	// Setup logger
	Set_Logger("HW4_Q4_L", LOG_DEBUG);

	printf("\nThis Program uses Syslog instead of printf\n");
	printf("\nExecute following to see the output:\n\ncd /var/log && grep HW4_Q4_L syslog\n");

	dev_name = "/dev/video0";

	syslog (LOG_INFO, ">>>>>>>>>> Program Start <<<<<<<<<<");

	open_device();
	init_device();
	start_capturing();

    //Setup the system for realtime execution
	if(Realtime_Setup() != 0)
	{
        	exit(-1);
	}

	// Give second highest priority to the Logger thread, Bind it to CPU 0, and create it
	Bind_to_CPU(0);
	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Logger_Thread, &Attr_All, Logger_Func, (void *)0);

	// Give second highest priority to the Cam_Monitor thread, Bind it to CPU 3, and create it
	Bind_to_CPU(3);
	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Cam_Monitor_Thread, &Attr_All, Cam_Monitor_Func, (void *)0);

	// Give fifth highest priority to the Cam_Compress thread, Bind it to CPU 3, and create it
	Bind_to_CPU(3);
	Attr_Sch.sched_priority = FIFO_Max_Prio - 4;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Cam_Grey_Thread, &Attr_All, Cam_Grey_Func, (void *)0);

	// Give second highest priority to the Scheduler thread, Bind it to CPU 1, and create it
/*	Bind_to_CPU(1);
	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Scheduler_Thread, &Attr_All, Scheduler_Func, (void *)0);

	// Give third highest priority to the Cam_PPM thread, Bind it to CPU 3, and create it
	Bind_to_CPU(3);
	Attr_Sch.sched_priority = FIFO_Max_Prio - 2;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Cam_RGB_Thread, &Attr_All, Cam_RGB_Func, (void *)0);

	// Give fourth highest priority to the Cam_Compress thread, Bind it to CPU 3, and create it
	Bind_to_CPU(3);
	Attr_Sch.sched_priority = FIFO_Max_Prio - 3;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Cam_Compress_Thread, &Attr_All, Cam_Compress_Func, (void *)0);

	// Give second highest priority to the Cam_Socket thread, Bind it to CPU 2, and create it
	Bind_to_CPU(2);
	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Cam_Socket_Thread, &Attr_All, Cam_Socket_Func, (void *)0);*/

	// Wait for all threads to exit
	pthread_join(Logger_Thread, 0);
	pthread_join(Cam_Monitor_Thread, 0);
	pthread_join(Cam_Grey_Thread, 0);

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Close the log
	closelog();

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);

	stop_capturing();
	uninit_device();
	close_device();

	fprintf(stderr, "\n");

	// Exit the program
	exit(0);
}
