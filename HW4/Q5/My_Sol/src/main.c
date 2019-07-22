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

uint8_t mode = 0;
uint8_t res = 0;
uint32_t HRES, VRES;

//const uint32_t Horiz_Resolutions[5] = {960, 800, 640, 320, 160};
//const uint32_t Vert_Resolutions[5] = {720, 600, 480, 240, 120};

float Grey_Avg_FPS[5] = {0}, Grey_WCET[5] = {0}, Grey_Avg_ET[5] = {0};
float Bright_Avg_FPS[5] = {0}, Bright_WCET[5] = {0}, Bright_Avg_ET[5] = {0};
float Contrast_Avg_FPS[5] = {0}, Contrast_WCET[5] = {0}, Contrast_Avg_ET[5] = {0};

float Grey_Exec_Time[No_of_Frames] = {0}, Bright_Exec_Time[No_of_Frames] = {0}, Contrast_Exec_Time[No_of_Frames] = {0};

int main (int argc, char *argv[])
{
	uint8_t i, j;
	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

	// Setup logger
	Set_Logger("HW4_Q5_1", LOG_DEBUG);

	printf("\nThis Program uses Syslog instead of printf\n");
	printf("\nExecute following to see the output:\n\ncd /var/log && grep HW4_Q5_1 syslog\n");

	dev_name = "/dev/video0";

	syslog (LOG_INFO, ">>>>>>>>>> Program Start <<<<<<<<<<");

    	//Setup the system for realtime execution
	if(Realtime_Setup() != 0)
	{
		exit(-1);
	}

	mq_unlink(grey_q_name);
	mq_unlink(bright_q_name);
	mq_unlink(contrast_q_name);	
	mq_unlink(monitor_q_name);

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

		printf("\n*** Started on Resolution: %d*%d ***", HRES, VRES);

		mode = 0;

		for(j = 0; j < 3; j ++)
		{
			open_device();
			init_device();
			start_capturing();
			// Give second highest priority to the Cam_Monitor thread, Bind it to CPU 3, and create it
			Bind_to_CPU(3);
			Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
			pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
			pthread_create(&Cam_Monitor_Thread, &Attr_All, Cam_Monitor_Func, (void *)0);

			switch(mode)
			{
				case 0:
				{
					Bind_to_CPU(3);
					Attr_Sch.sched_priority = FIFO_Max_Prio - 2;
					pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
					pthread_create(&Cam_Grey_Thread, &Attr_All, Cam_Grey_Func, (void *)0);

					pthread_join(Cam_Grey_Thread, 0); 

					printf("\nGrey_Avg_fps: %.3f\nGrey_Avg_ET: %.6fus\nGrey_WCET: %.6fus\n", Grey_Avg_FPS[res], Grey_Avg_ET[res], Grey_WCET[res]);
		
					break;
				}

				case 1:
				{
					Bind_to_CPU(3);
					Attr_Sch.sched_priority = FIFO_Max_Prio - 3;
					pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
					pthread_create(&Cam_Brightness_Thread, &Attr_All, Cam_Brightness_Func, (void *)0);

					pthread_join(Cam_Brightness_Thread, 0); 

					printf("\nBright_Avg_fps: %.3f\nBright_Avg_ET: %.6fus\nBright_WCET: %.6fus\n", Bright_Avg_FPS[res], Bright_Avg_ET[res], Bright_WCET[res]);
		
					break;
				}

				case 2:
				{
					Bind_to_CPU(3);
					Attr_Sch.sched_priority = FIFO_Max_Prio - 4;
					pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
					pthread_create(&Cam_Contrast_Thread, &Attr_All, Cam_Contrast_Func, (void *)0);

					pthread_join(Cam_Contrast_Thread, 0);

					printf("\nContrast_Avg_fps: %.3f\nContrast_Avg_ET: %.6fus\nContrast_WCET: %.6fus\n", Contrast_Avg_FPS[res], Contrast_Avg_ET[res], Contrast_WCET[res]);
		
					break;
				}

				default:
				{
					break;
				}
			}

			pthread_join(Cam_Monitor_Thread, 0);

			stop_capturing();
			uninit_device();
			close_device();
	
			mode += 1;
		}

		fprintf(stderr, "\n");

		printf("\n*** Ended on Resolution: %d*%d ***\n", HRES, VRES);

		res += 1;

		mq_unlink(grey_q_name);
		mq_unlink(bright_q_name);
		mq_unlink(contrast_q_name);
		mq_unlink(monitor_q_name); 

	} 

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);

	// Close the log
	closelog();


	// Exit the program
	exit(0);
}
