#include "main.h"

extern uint8_t Terminate_Flag;

extern uint8_t res;

extern float ref_time;

extern float Grey_Exec_Rem[], Bright_Exec_Rem[], Contrast_Exec_Rem[];
extern float Grey_Avg_Exec_Rem[5], Bright_Avg_Exec_Rem[5], Contrast_Avg_Exec_Rem[5];
extern float Grey_Max_Exec_Rem[5], Bright_Max_Exec_Rem[5], Contrast_Max_Exec_Rem[5];
extern float Grey_Min_Exec_Rem[5], Bright_Min_Exec_Rem[5], Contrast_Min_Exec_Rem[5];
extern float Avg_FPS[5];
extern uint32_t Grey_Missed_Deadlines[5], Bright_Missed_Deadlines[5], Contrast_Missed_Deadlines[5]; 

extern uint8_t Complete_Var;
	
extern sem_t Sched_Sem, Monitor_Sem, Grey_Sem, Brightness_Sem, Contrast_Sem;

extern struct timespec start_time_1, stop_time_1;

static uint32_t frames = 0;

static float difference_ms_1 = 0, frame_timings = 0, frame_rate_1 = 0;

extern uint32_t HRES, VRES;

static uint32_t i, j;
static float Exec_Rem_Sum;

static int resp;

static uint32_t sleep_time, sleep_counter, sig_counter;

static struct itimerval custom_timer;

static char str[20];

//static sig_atomic_t sig_flag;

static void signal_function(int value)
{
	if(value == SIGALRM)
	{
		sem_post(&Sched_Sem);
	}
}

static void sig_setup(void)
{
		// Configuring timer and signal action
		struct sigaction custom_signal_action;

		// Set all initial values to 0 in the structure
		memset(&custom_signal_action, 0, sizeof (custom_signal_action));

		// Set signal action handler to point to the address of the target function (to execute on receiving signal)
		custom_signal_action.sa_handler = &signal_function;

		// Setting interval according to define in main.h
		custom_timer.it_interval.tv_sec = 0;
		custom_timer.it_interval.tv_usec = (0.96 * Deadline_ms * ms_to_us);

		// Setting initial delay
		custom_timer.it_value.tv_sec = 0;
		custom_timer.it_value.tv_usec = (0.96 * Deadline_ms * ms_to_us);

		// Setting the signal action to kick in the handler function for following signals
		if(sigaction(SIGALRM, &custom_signal_action, 0) != 0)
		{
			syslog (LOG_ERR, "<%.6fms>!!Scheduler!! Sigaction Error", Time_Stamp(Mode_ms));
		}

		// Starting timer
		if(setitimer(ITIMER_REAL, &custom_timer, 0) != 0)
		{
			syslog (LOG_ERR, "<%.6fms>!!Scheduler!! Timer Error", Time_Stamp(Mode_ms));
		}
}

void *Scheduler_Func(void *para_t)
{

	struct timespec diff_t, rem_t;
	float tcalc;

	frame_timings = 0;
	difference_ms_1 = 0;
	frame_rate_1 = 0;
	Complete_Var = Process_Complete_Mask;
	frames = 0;
	Terminate_Flag = 0;
	sleep_time = 0;
	sleep_counter = 0;
	sig_counter = 0;

	ref_time = Time_Stamp(Mode_us);

	syslog (LOG_INFO, "<%.6fms>!!Scheduler!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

//	sig_setup();

	syslog (LOG_INFO, "<%.6fms>!!Scheduler!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	while(frames < No_of_Frames)
	{

		sem_post(&Monitor_Sem);

		diff_t.tv_sec = 0;
		diff_t.tv_nsec = ms_to_ns;

		do
		{
			resp = clock_nanosleep(CLOCK_REALTIME, 0, &diff_t, &rem_t);
			diff_t.tv_nsec = rem_t.tv_nsec;
		}while(resp != 0);

//		sem_wait(&Sched_Sem);

		sem_post(&Grey_Sem);
		sem_post(&Brightness_Sem);
		sem_post(&Contrast_Sem);

		tcalc = Time_Stamp(Mode_us);
		tcalc -= ref_time;
		tcalc -= (float)(sleep_counter * (Deadline_ms * ms_to_us));
		sleep_time = ((float)(0.99 * Deadline_ms * ms_to_us)) - tcalc;
//		sleep_time = ((float)(0.99 * Deadline_ms * ms_to_us)) - (Time_Stamp(Mode_us) - ((float)(sleep_counter * (Deadline_ms * ms_to_us))) - ref_time);

		diff_t.tv_sec = 0;
		diff_t.tv_nsec = sleep_time * us_to_ns;
//		diff_t.tv_nsec = (0.99 * Deadline_ms * ms_to_ns);

		do
		{
			resp = clock_nanosleep(CLOCK_REALTIME, 0, &diff_t, &rem_t);
			diff_t.tv_nsec = rem_t.tv_nsec;
		}while(resp != 0);

//		sem_wait(&Sched_Sem);

		if((Complete_Var & Total_Complete) == Total_Complete)
		{
			clock_gettime(CLOCK_REALTIME, &stop_time_1);

			if(stop_time_1.tv_nsec >= start_time_1.tv_nsec)
			{
				difference_ms_1 = ((stop_time_1.tv_sec - start_time_1.tv_sec) * 1000) + (((float)(stop_time_1.tv_nsec - start_time_1.tv_nsec)) / 1000000);
			}

			else
			{
				difference_ms_1 = ((stop_time_1.tv_sec - start_time_1.tv_sec - 1) * 1000) + (((float)(1000000000 - (start_time_1.tv_nsec - stop_time_1.tv_nsec))) / 1000000);
			}

			frame_timings += ((float)1000 / difference_ms_1);

			Complete_Var = Process_Complete_Mask;
			
			frames += 1;
		}
/*		else
		{
			printf("\nException... Transforms didn't finish");
		}*/

		sleep_counter += 1;
	}

	custom_timer.it_interval.tv_sec = 0;
	custom_timer.it_interval.tv_usec = 0;

	Terminate_Flag = 1;

	sem_post(&Monitor_Sem);
	sem_post(&Grey_Sem);
	sem_post(&Brightness_Sem);
	sem_post(&Contrast_Sem);

	// GREY

	i = 0;

	while(Grey_Exec_Rem[i] != 0)
	{
		i += 1;
	}

	Exec_Rem_Sum = 0;

	for(j = 0; j < i; j ++)
	{
		Exec_Rem_Sum += Grey_Exec_Rem[j];

		if(Grey_Exec_Rem[j] > Grey_Max_Exec_Rem[res])
		{
			Grey_Max_Exec_Rem[res] = Grey_Exec_Rem[j];
		}
		
		if(Grey_Exec_Rem[j] < Grey_Min_Exec_Rem[res])
		{
			Grey_Min_Exec_Rem[res] = Grey_Exec_Rem[j];

			if(Grey_Min_Exec_Rem[res] < 0)
			{
				Grey_Missed_Deadlines[res] += 1;
			}
		}

		switch(res)
		{
			case 0:
			{
				strncpy(str, "JITTER_GREY_R0", sizeof(str));
				break;
			}

			case 1:
			{
				strncpy(str, "JITTER_GREY_R1", sizeof(str));
				break;
			}

			case 2:
			{
				strncpy(str, "JITTER_GREY_R2", sizeof(str));
				break;
			}

			case 3:
			{
				strncpy(str, "JITTER_GREY_R3", sizeof(str));
				break;
			}

			case 4:
			{
				strncpy(str, "JITTER_GREY_R4", sizeof(str));
				break;
			}

			default:
			{
				break;
			}
		}
			
		syslog (LOG_INFO, "!!Schedule!! %s: %.6f", str, Grey_Exec_Rem[j]);

	}	

	Grey_Avg_Exec_Rem[res] = (Exec_Rem_Sum / ((float)(i + 1)));

	// BRIGHT

	i = 0;

	while(Bright_Exec_Rem[i] != 0)
	{
		i += 1;
	}

	Exec_Rem_Sum = 0;

	for(j = 0; j < i; j ++)
	{
		Exec_Rem_Sum += Bright_Exec_Rem[j];

		if(Bright_Exec_Rem[j] > Bright_Max_Exec_Rem[res])
		{
			Bright_Max_Exec_Rem[res] = Bright_Exec_Rem[j];
		}
		
		if(Bright_Exec_Rem[j] < Bright_Min_Exec_Rem[res])
		{
			Bright_Min_Exec_Rem[res] = Bright_Exec_Rem[j];

			if(Bright_Min_Exec_Rem[res] < 0)
			{
				Bright_Missed_Deadlines[res] += 1;
			}
		}

		switch(res)
		{
			case 0:
			{
				strncpy(str, "JITTER_BRIGHT_R0", sizeof(str));
				break;
			}

			case 1:
			{
				strncpy(str, "JITTER_BRIGHT_R1", sizeof(str));
				break;
			}

			case 2:
			{
				strncpy(str, "JITTER_BRIGHT_R2", sizeof(str));
				break;
			}

			case 3:
			{
				strncpy(str, "JITTER_BRIGHT_R3", sizeof(str));
				break;
			}

			case 4:
			{
				strncpy(str, "JITTER_BRIGHT_R4", sizeof(str));
				break;
			}

			default:
			{
				break;
			}
		}
			
		syslog (LOG_INFO, "!!Schedule!! %s: %.6f", str, Bright_Exec_Rem[j]);

	}	

	Bright_Avg_Exec_Rem[res] = (Exec_Rem_Sum / ((float)(i + 1)));

	// CONTRAST

	i = 0;

	while(Contrast_Exec_Rem[i] != 0)
	{
		i += 1;
	}

	Exec_Rem_Sum = 0;

	for(j = 0; j < i; j ++)
	{
		Exec_Rem_Sum += Contrast_Exec_Rem[j];

		if(Contrast_Exec_Rem[j] > Contrast_Max_Exec_Rem[res])
		{
			Contrast_Max_Exec_Rem[res] = Contrast_Exec_Rem[j];
		}
		
		if(Contrast_Exec_Rem[j] < Contrast_Min_Exec_Rem[res])
		{
			Contrast_Min_Exec_Rem[res] = Contrast_Exec_Rem[j];

			if(Contrast_Min_Exec_Rem[res] < 0)
			{
				Contrast_Missed_Deadlines[res] += 1;
			}
		}

		switch(res)
		{
			case 0:
			{
				strncpy(str, "JITTER_CONTRAST_R0", sizeof(str));
				break;
			}

			case 1:
			{
				strncpy(str, "JITTER_CONTRAST_R1", sizeof(str));
				break;
			}

			case 2:
			{
				strncpy(str, "JITTER_CONTRAST_R2", sizeof(str));
				break;
			}

			case 3:
			{
				strncpy(str, "JITTER_CONTRAST_R3", sizeof(str));
				break;
			}

			case 4:
			{
				strncpy(str, "JITTER_CONTRAST_R4", sizeof(str));
				break;
			}

			default:
			{
				break;
			}
		}
			
		syslog (LOG_INFO, "!!Schedule!! %s: %.6f", str, Contrast_Exec_Rem[j]);

	}	

	Contrast_Avg_Exec_Rem[res] = (Exec_Rem_Sum / ((float)(i + 1)));

	frame_rate_1 = (frame_timings / ((float)No_of_Frames));

	Avg_FPS[res] = frame_rate_1;

	syslog (LOG_INFO, "<%.6fms>!!Scheduler!! *****Average FPS for Resolution: %d*%d is: %.3f*****", Time_Stamp(Mode_ms), HRES, VRES, frame_rate_1);

	usleep(100000);
	pthread_exit(0);
}
