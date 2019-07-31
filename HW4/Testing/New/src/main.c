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
pthread_t Scheduler_Thread, Cam_Brightness_Thread, Cam_Monitor_Thread, Storage_Thread, Socket_Thread;

// Thread attribute variable
pthread_attr_t Attr_All;

// Scheduler parameter
struct sched_param Attr_Sch;

// Realtime Priority variables for FIFO
uint8_t FIFO_Max_Prio = 99, FIFO_Min_Prio = 1;

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

uint8_t res = 0;
uint32_t HRES, VRES;

uint8_t Terminate_Flag = 0;

uint8_t Complete_Var = 0;

float ref_time;
	
sem_t Brightness_Sem, Monitor_Sem;

frame_p_buffer shared_struct;

uint8_t data_buffer[No_of_Buffers][Big_Buffer_Size];

char target_ip[20];

int main (int argc, char *argv[])
{

	if(argc > 1)
	{
		strcpy(target_ip, argv[1]);
		printf("IP entered - trying to connect to: %s\n", target_ip);
	}
	else
	{
		strcpy(target_ip, Default_IP);
		printf("No IP entered - will connect to default: %s\n", target_ip);
	}

	uint32_t i;
	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

	// Setup logger
	Set_Logger("Sock_Test", LOG_DEBUG);

	printf("\nThis Program uses Syslog instead of printf\n");
	printf("\nExecute following to see the output:\n\ncd /var/log && grep Sock_Test syslog\n");

	dev_name = "/dev/video0";

	res = 2;

	syslog (LOG_INFO, ">>>>>>>>>> Program Start <<<<<<<<<<");

    	//Setup the system for realtime execution
	if(Realtime_Setup() != 0)
	{
		exit(-1);
	}
/*
	FILE *fp;
	fp = fopen("fps_analysis.txt", "a+");

	fprintf(fp, "\n>>>>>>>>>>FPS Analysis (No Accumulation, Frames: %d) <<<<<<<<<<\n", No_of_Frames);*/

	for(i = 0; i < Iterations; i ++)
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

		if(sem_init(&Monitor_Sem, 0, 0) != 0)
		{
		    syslog(LOG_ERR, "Failed to initialize Monitor_Sem semaphore");
		    exit(-1);
		}

		if(sem_init(&Brightness_Sem, 0, 0) != 0)
		{
		    syslog(LOG_ERR, "Failed to initialize Brightness_Sem semaphore");
		    exit(-1);
		}

		printf("\n\n*** Started on Resolution: %d*%d ***\n", HRES, VRES);

		open_device();
		init_device();
		start_capturing();

		device_warmup();
//		device_warmup();
//		device_warmup();

		usleep(10000);

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
		pthread_create(&Cam_Brightness_Thread, &Attr_All, Cam_Brightness_Func, (void *)0);

		Bind_to_CPU(2);
		Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
		pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
		pthread_create(&Storage_Thread, &Attr_All, Storage_Func, (void *)0);

		Bind_to_CPU(1);
		Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
		pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
		pthread_create(&Socket_Thread, &Attr_All, Socket_Func, (void *)0); 

		pthread_join(Cam_Monitor_Thread, 0); 
		pthread_join(Cam_Brightness_Thread, 0);
		pthread_join(Scheduler_Thread, 0); 
		pthread_join(Storage_Thread, 0);
		pthread_join(Socket_Thread, 0);

		stop_capturing();
		uninit_device();
		close_device();


		fprintf(stderr, "\n");

		printf("\n*** Ended on Resolution: %d*%d ***\n\n", HRES, VRES);

		sem_destroy(&Brightness_Sem);

	} 

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);

	// Close the log
	closelog();

	// Exit the program
	exit(0);
}
