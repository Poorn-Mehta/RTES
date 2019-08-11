/*
*		File: Aux_Func.c
*		Purpose: The source file containing Auxiliary functions related to the functionalities required
*		Owner: Poorn Mehta
*		Last Modified: 6/13/2019
*/

#include "main.h"
#include "Aux_Func.h"

// Shared variables
struct timespec start_time;
pthread_attr_t Attr_All;
struct sched_param Attr_Sch;
uint8_t FIFO_Max_Prio, FIFO_Min_Prio;
cpu_set_t CPU_Core;
strct_analyze Analysis;
uint32_t Target_FPS, Deadline_ms, Scheduler_Deadline, Monitor_Deadline, sch_index, HRES, VRES;

static uint32_t i;
static float sum_1, sum_2, sum_3, sum_4, sum_5, sum_6;

uint8_t Select_Resolution(uint8_t Res_Setting)
{
	switch(Res_Setting)
	{
		case Res_960_720:
		{
			HRES = 960;
			VRES = 720;
			break;
		}
		
		case Res_800_600:
		{
			HRES = 800;
			VRES = 600;
			break;
		}

		case Res_640_480:
		{
			HRES = 640;
			VRES = 480;
			break;
		}

		case Res_320_240:
		{
			HRES = 320;
			VRES = 240;
			break;
		}

		case Res_160_120:
		{
			HRES = 160;
			VRES = 120;
			break;
		}

		default:
		{
			HRES = 640;
			VRES = 480;
			break;
		}
	}

	return Res_Setting;
}

void Show_Analysis(void)
{

	sum_1 = 0;
	sum_4 = 0;

	for(i = 0; i < sch_index; i ++)
	{
		Analysis.Jitter_Analysis.Scheduler_Jitter[i] = Analysis.Exec_Analysis.Scheduler_Exec[i] - (float)Scheduler_Deadline;
		syslog (LOG_INFO, "!!Sch_Exec!! %.3f", Analysis.Exec_Analysis.Scheduler_Exec[i]);
		syslog (LOG_INFO, "!!Sch_Jitter!! %.3f", Analysis.Jitter_Analysis.Scheduler_Jitter[i]);

		sum_1 += Analysis.Jitter_Analysis.Scheduler_Jitter[i];

		if(Analysis.Jitter_Analysis.Max_Jitter[Scheduler_TID] <= fabs(Analysis.Jitter_Analysis.Scheduler_Jitter[i]))
		{
			Analysis.Jitter_Analysis.Max_Jitter[Scheduler_TID] = fabs(Analysis.Jitter_Analysis.Scheduler_Jitter[i]);
		}

		sum_4 += Analysis.Exec_Analysis.Scheduler_Exec[i];

		if(Analysis.Exec_Analysis.WCET[Scheduler_TID] <= fabs(Analysis.Exec_Analysis.Scheduler_Exec[i]))
		{
			Analysis.Exec_Analysis.WCET[Scheduler_TID] = fabs(Analysis.Exec_Analysis.Scheduler_Exec[i]);
		}
	}

	Analysis.Jitter_Analysis.Avg_Jitter[Scheduler_TID] = sum_1 / sch_index;

	Analysis.Exec_Analysis.Avg_Exec[Scheduler_TID] = sum_4 / sch_index;

	sum_1 = 0;
	sum_4 = 0;

	for(i = 0; i < Monitor_Loop_Count; i ++)
	{
		Analysis.Jitter_Analysis.Monitor_Jitter[i] = Analysis.Exec_Analysis.Monitor_Exec[i] - (float)Monitor_Deadline;
		syslog (LOG_INFO, "!!Mon_Exec!! %.3f", Analysis.Exec_Analysis.Monitor_Exec[i]);
		syslog (LOG_INFO, "!!Mon_Jitter!! %.3f", Analysis.Jitter_Analysis.Monitor_Jitter[i]);

		sum_1 += Analysis.Jitter_Analysis.Monitor_Jitter[i];

		if(Analysis.Jitter_Analysis.Max_Jitter[Monitor_TID] <= fabs(Analysis.Jitter_Analysis.Monitor_Jitter[i]))
		{
			Analysis.Jitter_Analysis.Max_Jitter[Monitor_TID] = fabs(Analysis.Jitter_Analysis.Monitor_Jitter[i]);
		}

		sum_4 += Analysis.Exec_Analysis.Monitor_Exec[i];

		if(Analysis.Exec_Analysis.WCET[Monitor_TID] <= fabs(Analysis.Exec_Analysis.Monitor_Exec[i]))
		{
			Analysis.Exec_Analysis.WCET[Monitor_TID] = fabs(Analysis.Exec_Analysis.Monitor_Exec[i]);
		}
	}

	Analysis.Jitter_Analysis.Avg_Jitter[Monitor_TID] = sum_1 / Monitor_Loop_Count;

	Analysis.Exec_Analysis.Avg_Exec[Monitor_TID] = sum_4 / Monitor_Loop_Count;

	sum_1 = 0;
	sum_2 = 0;
	sum_3 = 0;
	sum_4 = 0;
	sum_5 = 0;
	sum_6 = 0;

	for(i = 0; i < No_of_Frames; i ++)
	{
		Analysis.Jitter_Analysis.RGB_Jitter[i] = Analysis.Exec_Analysis.RGB_Exec[i] - (float)Deadline_ms;
		syslog (LOG_INFO, "!!Brgt_Exec!! %.3f", Analysis.Exec_Analysis.RGB_Exec[i]);
		syslog (LOG_INFO, "!!Brgt_Jitter!! %.3f", Analysis.Jitter_Analysis.RGB_Jitter[i]);

		sum_1 += Analysis.Jitter_Analysis.RGB_Jitter[i];

		if(Analysis.Jitter_Analysis.Max_Jitter[RGB_TID] <= fabs(Analysis.Jitter_Analysis.RGB_Jitter[i]))
		{
			Analysis.Jitter_Analysis.Max_Jitter[RGB_TID] = fabs(Analysis.Jitter_Analysis.RGB_Jitter[i]);
		}

		sum_4 += Analysis.Exec_Analysis.RGB_Exec[i];

		if(Analysis.Exec_Analysis.WCET[RGB_TID] <= fabs(Analysis.Exec_Analysis.RGB_Exec[i]))
		{
			Analysis.Exec_Analysis.WCET[RGB_TID] = fabs(Analysis.Exec_Analysis.RGB_Exec[i]);
		}

		Analysis.Jitter_Analysis.Storage_Jitter[i] = Analysis.Exec_Analysis.Storage_Exec[i] - (float)Deadline_ms;
		syslog (LOG_INFO, "!!Store_Exec!! %.3f", Analysis.Exec_Analysis.Storage_Exec[i]);
		syslog (LOG_INFO, "!!Store_Jitter!! %.3f", Analysis.Jitter_Analysis.Storage_Jitter[i]);

		sum_2 += Analysis.Jitter_Analysis.Storage_Jitter[i];

		if(Analysis.Jitter_Analysis.Max_Jitter[Storage_TID] <= fabs(Analysis.Jitter_Analysis.Storage_Jitter[i]))
		{
			Analysis.Jitter_Analysis.Max_Jitter[Storage_TID] = fabs(Analysis.Jitter_Analysis.Storage_Jitter[i]);
		}

		sum_5 += Analysis.Exec_Analysis.Storage_Exec[i];

		if(Analysis.Exec_Analysis.WCET[Storage_TID] <= fabs(Analysis.Exec_Analysis.Storage_Exec[i]))
		{
			Analysis.Exec_Analysis.WCET[Storage_TID] = fabs(Analysis.Exec_Analysis.Storage_Exec[i]);
		}

		Analysis.Jitter_Analysis.Socket_Jitter[i] = Analysis.Exec_Analysis.Socket_Exec[i] - (float)Deadline_ms;
		syslog (LOG_INFO, "!!Sock_Exec!! %.3f", Analysis.Exec_Analysis.Socket_Exec[i]);
		syslog (LOG_INFO, "!!Sock_Jitter!! %.3f", Analysis.Jitter_Analysis.Socket_Jitter[i]);

		sum_3 += Analysis.Jitter_Analysis.Socket_Jitter[i];

		if(Analysis.Jitter_Analysis.Max_Jitter[Socket_TID] <= fabs(Analysis.Jitter_Analysis.Socket_Jitter[i]))
		{
			Analysis.Jitter_Analysis.Max_Jitter[Socket_TID] = fabs(Analysis.Jitter_Analysis.Socket_Jitter[i]);
		}

		sum_6 += Analysis.Exec_Analysis.Socket_Exec[i];

		if(Analysis.Exec_Analysis.WCET[Socket_TID] <= fabs(Analysis.Exec_Analysis.Socket_Exec[i]))
		{
			Analysis.Exec_Analysis.WCET[Socket_TID] = fabs(Analysis.Exec_Analysis.Socket_Exec[i]);
		}
	}

	Analysis.Jitter_Analysis.Avg_Jitter[RGB_TID] = sum_1 / No_of_Frames;
	Analysis.Jitter_Analysis.Avg_Jitter[Storage_TID] = sum_2 / No_of_Frames;
	Analysis.Jitter_Analysis.Avg_Jitter[Socket_TID] = sum_3 / No_of_Frames;

	Analysis.Exec_Analysis.Avg_Exec[RGB_TID] = sum_4 / No_of_Frames;
	Analysis.Exec_Analysis.Avg_Exec[Storage_TID] = sum_5 / No_of_Frames;
	Analysis.Exec_Analysis.Avg_Exec[Socket_TID] = sum_6 / No_of_Frames;

	printf("\n\n**********Detailed Analysis Below**********\n");

	printf("\n---GREP_INFO---\n");
	printf("\ngrep -a Sch_Exec syslog\ngrep -a Sch_Jitter syslog\ngrep -a Mon_Exec syslog\ngrep -a Mon_Jitter syslog");
	printf("\ngrep -a Brgt_Exec syslog\ngrep -a Brgt_Jitter syslog\ngrep -a Store_Exec syslog\ngrep -a Store_Jitter syslog\ngrep -a Sock_Exec syslog\ngrep -a Sock_Jitter syslog");

	printf("\n\nMax FPS: %.3f", Analysis.Max_FPS);
	printf("\nProgram Ran at %.3f FPS (Target FPS: %.3f)", Analysis.Running_FPS, (float)Target_FPS);

	printf("\n\nTotal Missed Deadline(s): %u (Total Frames: %d)", Analysis.Missed_Deadlines, No_of_Frames);

	printf("\n\nAverage Execution Time of Scheduler: %.3fms (Deadline: %.3fms)", Analysis.Exec_Analysis.Avg_Exec[Scheduler_TID], (float)Scheduler_Deadline);
	printf("\nWorst Case Execution Time of Scheduler: %.3fms", Analysis.Exec_Analysis.WCET[Scheduler_TID]);

	printf("\n\nAverage Execution Time of Monitor: %.3fms (Deadline: %.3fms)", Analysis.Exec_Analysis.Avg_Exec[Monitor_TID], (float)Monitor_Deadline);
	printf("\nWorst Case Execution Time of Monitor: %.3fms", Analysis.Exec_Analysis.WCET[Monitor_TID]);

	printf("\n\nAverage Execution Time of RGB: %.3fms (Deadline: %.3fms)", Analysis.Exec_Analysis.Avg_Exec[RGB_TID], (float)Deadline_ms);
	printf("\nWorst Case Execution Time of RGB: %.3fms", Analysis.Exec_Analysis.WCET[RGB_TID]);

	printf("\n\nAverage Execution Time of Storage: %.3fms (Goal: %.3fms)", Analysis.Exec_Analysis.Avg_Exec[Storage_TID], (float)Deadline_ms);
	printf("\nWorst Case Execution Time of Storage: %.3fms", Analysis.Exec_Analysis.WCET[Storage_TID]);

	printf("\n\nAverage Execution Time of Socket: %.3fms (Goal: %.3fms)", Analysis.Exec_Analysis.Avg_Exec[Socket_TID], (float)Deadline_ms);
	printf("\nWorst Case Execution Time of Socket: %.3fms", Analysis.Exec_Analysis.WCET[Socket_TID]);

	printf("\n\nOverall Jitter/Deviation of Scheduler: %.3fms", Analysis.Jitter_Analysis.Overall_Jitter[Scheduler_TID]);
	printf("\nAverage Jitter of Scheduler: %.3fms", Analysis.Jitter_Analysis.Avg_Jitter[Scheduler_TID]);
	printf("\nMaximum (Absolute) Jitter of Scheduler: %.3fms", Analysis.Jitter_Analysis.Max_Jitter[Scheduler_TID]);

	printf("\n\nOverall Jitter/Deviation of Monitor: %.3fms", Analysis.Jitter_Analysis.Overall_Jitter[Monitor_TID]);
	printf("\nAverage Jitter of Monitor: %.3fms", Analysis.Jitter_Analysis.Avg_Jitter[Monitor_TID]);
	printf("\nMaximum (Absolute) Jitter of Monitor: %.3fms", Analysis.Jitter_Analysis.Max_Jitter[Monitor_TID]);

	printf("\n\nOverall Jitter/Deviation of RGB: %.3fms", Analysis.Jitter_Analysis.Overall_Jitter[RGB_TID]);
	printf("\nAverage Jitter of RGB: %.3fms", Analysis.Jitter_Analysis.Avg_Jitter[RGB_TID]);
	printf("\nMaximum (Absolute) Jitter of RGB: %.3fms", Analysis.Jitter_Analysis.Max_Jitter[RGB_TID]);

	printf("\n\nOverall Jitter/Deviation of Storage: %.3fms", Analysis.Jitter_Analysis.Overall_Jitter[Storage_TID]);
	printf("\nAverage Jitter of Storage: %.3fms", Analysis.Jitter_Analysis.Avg_Jitter[Storage_TID]);
	printf("\nMaximum (Absolute) Jitter of Storage: %.3fms", Analysis.Jitter_Analysis.Max_Jitter[Storage_TID]);

	printf("\n\nOverall Jitter/Deviation of Socket: %.3fms", Analysis.Jitter_Analysis.Overall_Jitter[Socket_TID]);
	printf("\nAverage Jitter of Socket: %.3fms", Analysis.Jitter_Analysis.Avg_Jitter[Socket_TID]);
	printf("\nMaximum (Absolute) Jitter of Socket: %.3fms", Analysis.Jitter_Analysis.Max_Jitter[Socket_TID]);

	printf("\n\n");

}

// Function to return relative timestamps
float Time_Stamp(uint8_t mode)
{
	struct timespec curr_time = {0, 0};
	uint32_t multiplier;
	float inv_multiplier;
	switch(mode)
	{
		case Mode_sec:
		{
			multiplier = 1;
			inv_multiplier = ns_to_s;
			break;
		}

		case Mode_ms:
		{
			multiplier = s_to_ms;
			inv_multiplier = ns_to_ms;
			break;
		}

		case Mode_us:
		{
			multiplier = s_to_us;
			inv_multiplier = ns_to_us;
			break;
		}

		default:
		{
			multiplier = 1;
			inv_multiplier = ns_to_s;
			break;
		}
	}


	// Record current time
	clock_gettime(CLOCK_REALTIME, &curr_time);
	float rval = 0;

	// Calculate Difference
	if(curr_time.tv_nsec >= start_time.tv_nsec)
	{
        	rval = ((curr_time.tv_sec - start_time.tv_sec) * multiplier) + (((float)(curr_time.tv_nsec - start_time.tv_nsec)) * inv_multiplier);
	}

	else
	{
        	rval = ((curr_time.tv_sec - start_time.tv_sec - 1) * multiplier) + (((float)(s_to_ns - (start_time.tv_nsec - curr_time.tv_nsec))) * inv_multiplier);
	}

	return (rval);
}

// Setup Logger as needed
void Set_Logger(char *logname, int level_upto)
{
	// Logmask - the lowest (least important) Log Level to be written
	setlogmask(LOG_UPTO(level_upto));

	// Open log with prefix name
	openlog(logname, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
}

uint8_t Bind_to_CPU(uint8_t Core)
{
    // Bind thread to given CPU core
	if(Core < No_of_Cores)
	{
		// First, clearing variable
		CPU_ZERO(&CPU_Core);

		// Adding core-<Bind_to_CPU> in the variable
		CPU_SET(Core, &CPU_Core);

		// Executing API to set the CPU
		if(pthread_attr_setaffinity_np(&Attr_All, sizeof(cpu_set_t), &CPU_Core) != 0)
		{
			syslog(LOG_ERR, "Failed to Set CPU Core");
			return 1;
		}
		return 0;
	}
	return 1;
}

// Setting up system for realtime operation
uint8_t Realtime_Setup(void)
{
	int sched;

	// Initializing Attribute which will be passed to all pthreads
	if(pthread_attr_init(&Attr_All) != 0)
	{
		syslog(LOG_ERR, "Failed to Initialize Attr_All");
		return 1;
	}

	// Preventing pthreads to inherit scheduler, and tell them explicity to use given scheduler (through another API)
	if(pthread_attr_setinheritsched(&Attr_All, PTHREAD_EXPLICIT_SCHED) != 0)
	{
		syslog(LOG_ERR, "Failed to Set Schedule Inheritance");
		return 1;
	}

	// Set scheduling policy to FIFO for all threads
	if(pthread_attr_setschedpolicy(&Attr_All, SCHED_FIFO) != 0)
	{
		syslog(LOG_ERR, "Failed to Set Scheduling Policy");
		return 1;
	}

	// Getting Max and Min realtime priorities for FIFO
	FIFO_Max_Prio = sched_get_priority_max(SCHED_FIFO);
	FIFO_Min_Prio = sched_get_priority_min(SCHED_FIFO);

	// Setting scheduler for the program (by setting parent thread to FIFO) (I am not 100% sure about this though)
	Attr_Sch.sched_priority = FIFO_Max_Prio;
	if(sched_setscheduler(getpid(), SCHED_FIFO, &Attr_Sch) != 0)
	{
		perror("sched_setscheduler");
		syslog(LOG_ERR, "Failed to Set FIFO Scheduler");
		return 1;
	}

	// Verify that Scheduler is set to FIFO
	sched = sched_getscheduler(getpid());
	if(sched == SCHED_FIFO)		syslog (LOG_INFO, "Scheduling set to FIFO");
	else
	{
		syslog(LOG_ERR, "Scheduling is not FIFO");
		return 1;
	}

	return 0;

}
