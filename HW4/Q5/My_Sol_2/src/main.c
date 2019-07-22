/*
*		File: main.c
*		Purpose: The source file containing main() and global variables for Question 5 of Homework 3
*		Owner: Poorn Mehta
*		Last Modified: 7/5/2019
*		Linux Man pages has been referred whenever necessary. Also, I have referred to my own previous work (https://github.com/Poorn-Mehta/AESD) at some points.
*/

#include "main.h"

// To record start time
struct timespec start_time = {0, 0};

// Thread handles
pthread_t Scheduler_Thread, Logger_Thread, Cam_Socket_Thread, Cam_Brightness_Thread, Cam_Monitor_Thread, Cam_Contrast_Thread, Cam_Grey_Thread;

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
uint8_t Max_Throughput = 1;

uint8_t res = 0;
uint32_t HRES, VRES;

uint8_t Process_Complete;

uint8_t Terminate_Flag = 0;

uint8_t Complete_Var = 0;

float ref_time;
	
sem_t Sched_Sem, Monitor_Sem, Grey_Sem, Brightness_Sem, Contrast_Sem;

struct timespec start_time_1, stop_time_1;


float Grey_Avg_Exec_Rem[5], Bright_Avg_Exec_Rem[5], Contrast_Avg_Exec_Rem[5];
float Grey_Max_Exec_Rem[5], Bright_Max_Exec_Rem[5], Contrast_Max_Exec_Rem[5];
float Grey_Min_Exec_Rem[5], Bright_Min_Exec_Rem[5], Contrast_Min_Exec_Rem[5];
float Grey_Exec_Rem[(No_of_Frames * 3)], Bright_Exec_Rem[(No_of_Frames * 3)], Contrast_Exec_Rem[(No_of_Frames * 3)];
float Avg_FPS[5];
uint32_t Grey_No_Frame[5], Grey_Missed_Deadlines[5], Bright_No_Frame[5], Bright_Missed_Deadlines[5], Contrast_No_Frame[5], Contrast_Missed_Deadlines[5]; 

int main (int argc, char *argv[])
{
	uint32_t i;
	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

	// Setup logger
	Set_Logger("HW4_Q5_2_Log", LOG_DEBUG);

	printf("\nThis Program uses Syslog instead of printf\n");
	printf("\nExecute following to see the output:\n\ncd /var/log && grep HW4_Q5_2_Log syslog\n");

	dev_name = "/dev/video0";

	res = 0;

	for(i = 0; i < 5; i ++)
	{
		Grey_Avg_Exec_Rem[i] = 0;
		Bright_Avg_Exec_Rem[i] = 0;
		Contrast_Avg_Exec_Rem[i] = 0;
		Grey_Max_Exec_Rem[i] = 0;
		Bright_Max_Exec_Rem[i] = 0;
		Contrast_Max_Exec_Rem[i] = 0;
		Grey_Min_Exec_Rem[i] = (float)(Deadline_ms * ms_to_us);
		Bright_Min_Exec_Rem[i] = (float)(Deadline_ms * ms_to_us);
		Contrast_Min_Exec_Rem[i] = (float)(Deadline_ms * ms_to_us);
		Grey_No_Frame[i] = 0;
		Grey_Missed_Deadlines[i] = 0;
		Bright_No_Frame[i] = 0;
		Bright_Missed_Deadlines[i] = 0;
		Contrast_No_Frame[i] = 0;
		Contrast_Missed_Deadlines[i] = 0;
		Avg_FPS[i] = 0;
	}

	for(i = 0; i < (No_of_Frames * 3); i ++)
	{
		Grey_Exec_Rem[i] = 0;
		Bright_Exec_Rem[i] = 0;
		Contrast_Exec_Rem[i] = 0;
	}

	syslog (LOG_INFO, ">>>>>>>>>> Program Start <<<<<<<<<<");

    	//Setup the system for realtime execution
	if(Realtime_Setup() != 0)
	{
		exit(-1);
	}

	mq_unlink(grey_q_name);
	mq_unlink(bright_q_name);
	mq_unlink(contrast_q_name);	

	for(i = 0; i < 5; i ++)
	{			

		switch(res)
		{
			case 0:
			{
				HRES = 960;
				VRES = 720;
				break;
			}
			
			case 1:
			{
				HRES = 800;
				VRES = 600;
				break;
			}

			case 2:
			{
				HRES = 640;
				VRES = 480;
				break;
			}

			case 3:
			{
				HRES = 320;
				VRES = 240;
				break;
			}

			case 4:
			{
				HRES = 160;
				VRES = 120;
				break;
			}

			default:
			{
				break;
			}
		}

		if(sem_init(&Sched_Sem, 0, 0) != 0)
		{
		    syslog(LOG_ERR, "Failed to initialize Sched_Sem semaphore");
		    exit(-1);
		}

		if(sem_init(&Monitor_Sem, 0, 0) != 0)
		{
		    syslog(LOG_ERR, "Failed to initialize Monitor_Sem semaphore");
		    exit(-1);
		}

		if(sem_init(&Grey_Sem, 0, 0) != 0)
		{
		    syslog(LOG_ERR, "Failed to initialize Grey_Sem semaphore");
		    exit(-1);
		}

		if(sem_init(&Brightness_Sem, 0, 0) != 0)
		{
		    syslog(LOG_ERR, "Failed to initialize Brightness_Sem semaphore");
		    exit(-1);
		}

		if(sem_init(&Contrast_Sem, 0, 0) != 0)
		{
		    syslog(LOG_ERR, "Failed to initialize Contrast_Sem semaphore");
		    exit(-1);
		}

		printf("\n\n*** Started on Resolution: %d*%d ***\n", HRES, VRES);

		open_device();
		init_device();
		start_capturing();

		device_warmup();

		Bind_to_CPU(3);
		Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
		pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
		pthread_create(&Scheduler_Thread, &Attr_All, Scheduler_Func, (void *)0);

		Bind_to_CPU(3);
		Attr_Sch.sched_priority = FIFO_Max_Prio - 2;
		pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
		pthread_create(&Cam_Monitor_Thread, &Attr_All, Cam_Monitor_Func, (void *)0);

		Bind_to_CPU(3);
		Attr_Sch.sched_priority = FIFO_Max_Prio - 3;
		pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
		pthread_create(&Cam_Grey_Thread, &Attr_All, Cam_Grey_Func, (void *)0);

		Bind_to_CPU(3);
		Attr_Sch.sched_priority = FIFO_Max_Prio - 4;
		pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
		pthread_create(&Cam_Brightness_Thread, &Attr_All, Cam_Brightness_Func, (void *)0);

		Bind_to_CPU(3);
		Attr_Sch.sched_priority = FIFO_Max_Prio - 5;
		pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
		pthread_create(&Cam_Contrast_Thread, &Attr_All, Cam_Contrast_Func, (void *)0);

		pthread_join(Cam_Monitor_Thread, 0); 
		pthread_join(Cam_Grey_Thread, 0);
		pthread_join(Cam_Brightness_Thread, 0); 
		pthread_join(Cam_Contrast_Thread, 0);
		pthread_join(Scheduler_Thread, 0);

		stop_capturing();
		uninit_device();
		close_device();


		fprintf(stderr, "\n");

		printf("\n>>Grey Jitters (Remaining Execution Times relative to %dms Deadline)<<", Deadline_ms);
		printf("\nMin: %.6fus\nMax: %.6fus\nAvg: %.6fus\n", Grey_Min_Exec_Rem[res], Grey_Max_Exec_Rem[res], Grey_Avg_Exec_Rem[res]);
		printf("\nGrey No Frames: %d, Grey Missed Deadlines: %d\n", Grey_No_Frame[res], Grey_Missed_Deadlines[res]);

		printf("\n>>Brightness Jitters (Remaining Execution Times relative to %dms Deadline)<<", Deadline_ms);
		printf("\nMin: %.6fus\nMax: %.6fus\nAvg: %.6fus\n", Bright_Min_Exec_Rem[res], Bright_Max_Exec_Rem[res], Bright_Avg_Exec_Rem[res]);
		printf("\nBrightness No Frames: %d, Brightness Missed Deadlines: %d\n", Bright_No_Frame[res], Bright_Missed_Deadlines[res]);

		printf("\n>>Contrast Jitters (Remaining Execution Times relative to %dms Deadline)<<", Deadline_ms);
		printf("\nMin: %.6fus\nMax: %.6fus\nAvg: %.6fus\n", Contrast_Min_Exec_Rem[res], Contrast_Max_Exec_Rem[res], Contrast_Avg_Exec_Rem[res]);
		printf("\nContrast No Frames: %d, Contrast Missed Deadlines: %d\n", Contrast_No_Frame[res], Contrast_Missed_Deadlines[res]);

		printf("\nAverage FPS is: %.3f\n", Avg_FPS[res]);

		printf("\n*** Ended on Resolution: %d*%d ***\n\n", HRES, VRES);

		res += 1;

		mq_unlink(grey_q_name);
		mq_unlink(bright_q_name);
		mq_unlink(contrast_q_name); 

		sem_destroy(&Sched_Sem);
		sem_destroy(&Monitor_Sem);
		sem_destroy(&Grey_Sem);
		sem_destroy(&Brightness_Sem);
		sem_destroy(&Contrast_Sem);

	} 

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);

	// Close the log
	closelog();


	// Exit the program
	exit(0);
}
