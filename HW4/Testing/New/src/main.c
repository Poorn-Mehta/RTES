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

char target_ip[IP_Addr_Len];

struct timespec prev_t;

strct_analyze Analysis;

struct utsname sys_info;

uint32_t sch_index;

uint8_t Operating_Mode;
uint32_t Target_FPS = 1;
uint32_t Deadline_ms = 1000;
uint32_t Scheduler_Deadline = 10;
uint32_t Monitor_Deadline = 100;

float Monitor_Start_Stamp, Monitor_Stamp_1, Brightness_Start_Stamp, Brightness_Stamp_1;

int main (int argc, char *argv[])
{

	Operating_Mode = 0;

	if(argc > 3)
	{
		strncpy(target_ip, argv[3], IP_Addr_Len);
		printf("\nIP entered - trying to connect to: %s", target_ip);

		if((strncmp(Mode_Socket_OFF, argv[2], 5)) == 0)
		{
			Operating_Mode &= ~(Mode_Socket_Mask);
			printf("\nSocket is Turned OFF");
		}

		else if((strncmp(Mode_Socket_ON, argv[2], 5)) == 0)
		{
			Operating_Mode |= (Mode_Socket_Mask);
			printf("\nSocket is Turned ON");
		}

		else
		{
			Operating_Mode &= ~(Mode_Socket_Mask);
			printf("\nInvalid Argument, Socket is turned OFF by default");
		}

		if((strncmp(Mode_1_FPS, argv[1], 10)) == 0)
		{
			Operating_Mode &= ~(Mode_FPS_Mask);
			printf("\nSelected FPS: 1");
		}

		else if((strncmp(Mode_10_FPS, argv[1], 10)) == 0)
		{
			Operating_Mode |= (Mode_FPS_Mask);
			printf("\nSelected FPS: 10");
		}

		else
		{
			Operating_Mode &= ~(Mode_FPS_Mask);
			printf("\nInvalid Argument, Selected FPS: 1 by default");
		}
	}

	else if(argc > 2)
	{

		strncpy(target_ip, Default_IP, IP_Addr_Len);
		printf("\nNo IP entered - will connect to default: %s", target_ip);

		if((strncmp(Mode_Socket_OFF, argv[2], 5)) == 0)
		{
			Operating_Mode &= ~(Mode_Socket_Mask);
			printf("\nSocket is Turned OFF");
		}

		else if((strncmp(Mode_Socket_ON, argv[2], 5)) == 0)
		{
			Operating_Mode |= (Mode_Socket_Mask);
			printf("\nSocket is Turned ON");
		}

		else
		{
			Operating_Mode &= ~(Mode_Socket_Mask);
			printf("\nInvalid Argument, Socket is turned OFF by default");
		}

		if((strncmp(Mode_1_FPS, argv[1], 10)) == 0)
		{
			Operating_Mode &= ~(Mode_FPS_Mask);
			printf("\nSelected FPS: 1");
		}

		else if((strncmp(Mode_10_FPS, argv[1], 10)) == 0)
		{
			Operating_Mode |= (Mode_FPS_Mask);
			printf("\nSelected FPS: 10");
		}

		else
		{
			Operating_Mode &= ~(Mode_FPS_Mask);
			printf("\nInvalid Argument, Selected FPS: 1 by default");
		}
	}

	else if(argc > 1)
	{
		strncpy(target_ip, Default_IP, IP_Addr_Len);
		printf("No IP entered - will connect to default: %s\n", target_ip);

		Operating_Mode &= ~(Mode_FPS_Mask);
		printf("\nNo Argument for Socket, It is turned OFF by default");

		if((strncmp(Mode_1_FPS, argv[1], 10)) == 0)
		{
			Operating_Mode &= ~(Mode_FPS_Mask);
			printf("\nSelected FPS: 1");
		}

		else if((strncmp(Mode_10_FPS, argv[1], 10)) == 0)
		{
			Operating_Mode |= (Mode_FPS_Mask);
			printf("\nSelected FPS: 10");
		}

		else
		{
			Operating_Mode &= ~(Mode_FPS_Mask);
			printf("\nInvalid Argument, Selected FPS: 1 by default");
		}
	}

	else
	{
		strncpy(target_ip, Default_IP, IP_Addr_Len);
		printf("\nNo IP entered - will connect to default: %s", target_ip);

		Operating_Mode &= ~(Mode_Socket_Mask);
		printf("\nNo Argument for Socket, It is turned OFF by default");

		Operating_Mode &= ~(Mode_FPS_Mask);
		printf("\nNo Argument for FPS, Selected FPS: 1 by default");
	}

	if((Operating_Mode & Mode_FPS_Mask) == Mode_10_FPS_Val)
	{
		Target_FPS = 10;
	}

	else
	{
		Target_FPS = 1;
	}

	Deadline_ms = 1000 / Target_FPS;

	Scheduler_Deadline = Deadline_ms / Scheduler_Scaling_Factor;

	Monitor_Deadline = Deadline_ms / Monitor_Scaling_Factor;

	uint32_t i;
	// Record program start time, to provide relative time throughout the execution
	clock_gettime(CLOCK_REALTIME, &start_time);

	// Setup logger
	Set_Logger("Testing", LOG_DEBUG);

	printf("\n\nThis Program uses Syslog instead of printf\n");
	printf("\nExecute following to see the output:\n\ncd /var/log && grep -a Testing syslog\n");

	dev_name = "/dev/video0";

	res = 2;

	CLEAR(Analysis);

	syslog (LOG_INFO, ">>>>>>>>>> Program Start <<<<<<<<<<");

    	//Setup the system for realtime execution
	if(Realtime_Setup() != 0)
	{
		exit(-1);
	}

	if (uname(&sys_info) != 0)
	{
		perror("uname");
		exit(EXIT_FAILURE);
	}


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

		if(device_warmup() == 0)
		{
			syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Device Warmup Successful", Time_Stamp(Mode_ms));
		}

		else
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Device Warmup Failed", Time_Stamp(Mode_ms));
		}

		usleep(10000);

		if((Operating_Mode & Mode_FPS_Mask) == Mode_1_FPS_Val)
		{
			Cam_Filter();
			usleep(10000);
		}

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

		if((Operating_Mode & Mode_Socket_Mask) == Mode_Socket_ON_Val)
		{
			Bind_to_CPU(1);
			Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
			pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
			pthread_create(&Socket_Thread, &Attr_All, Socket_Func, (void *)0);
		}

		pthread_join(Cam_Monitor_Thread, 0); 
		pthread_join(Cam_Brightness_Thread, 0);
		pthread_join(Scheduler_Thread, 0); 
		pthread_join(Storage_Thread, 0);

		if((Operating_Mode & Mode_Socket_Mask) == Mode_Socket_ON_Val)
		{
			pthread_join(Socket_Thread, 0);
		}

		stop_capturing();
		uninit_device();
		close_device();


		fprintf(stderr, "\n");

		printf("\n*** Ended on Resolution: %d*%d ***\n\n", HRES, VRES);

		sem_destroy(&Brightness_Sem);
		sem_destroy(&Monitor_Sem);

	} 

	Show_Analysis();

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	// Destroy attribute
	pthread_attr_destroy(&Attr_All);

	// Close the log
	closelog();

	// Exit the program
	exit(0);
}
