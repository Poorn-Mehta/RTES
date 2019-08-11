#include "main.h"
#include "Aux_Func.h"
#include "Cam_RGB.h"
#include "Scheduler.h"

uint8_t Terminate_Flag, Operating_Mode;
uint32_t HRES, VRES, Scheduler_Deadline, Deadline_ms, sch_index;
uint8_t Complete_Var;	
sem_t Sched_Sem, RGB_Sem, Monitor_Sem;
struct timespec prev_t;
strct_analyze Analysis;
float Monitor_Start_Stamp, Monitor_Stamp_1, RGB_Start_Stamp, RGB_Stamp_1;

static uint32_t frames = 0, sch_cnt;
static int resp;
static uint8_t startup;
static float Running_FPS_Stamp_1, Running_FPS_Stamp_2, Scheduler_Stamp_1, Scheduler_Stamp_2;

void *Scheduler_Func(void *para_t)
{
	Complete_Var = 0;
	frames = 0;
	Terminate_Flag = 0;
	sch_cnt = 1;
	sch_index = 0;
	startup = 0;

	if(mq_unlink(storage_q_name) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Scheduler!! Couldn't unlink Storage Queue", Time_Stamp(Mode_ms));
	}

	if(mq_unlink(socket_q_name) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Scheduler!! Couldn't unlink Socket Queue", Time_Stamp(Mode_ms));
	}

	const uint32_t sleep_time = (Deadline_ms * ms_to_ns) / Scheduler_Scaling_Factor;

	syslog(LOG_INFO, "<%.6fms>!!Scheduler!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	if((Operating_Mode & Mode_FPS_Mask) == Mode_10_FPS_Val)
	{
		if(clock_gettime(CLOCK_REALTIME, &prev_t) != 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Scheduler!! Couldn't get time for prev_t", Time_Stamp(Mode_ms));
		}
	}

	while(frames < No_of_Frames)
	{

		syslog(LOG_INFO, "<%.6fms>!!RMA!! Scheduler Launched (Iteration: %d)", Time_Stamp(Mode_ms), sch_index);

		if(sch_index > 0)
		{
			Scheduler_Stamp_1 = Time_Stamp(Mode_ms);
		}

		if(prev_t.tv_nsec < (s_to_ns - sleep_time))
		{
			prev_t.tv_sec = prev_t.tv_sec;
			prev_t.tv_nsec = (uint32_t)(prev_t.tv_nsec + sleep_time);
		}

		else
		{
			prev_t.tv_sec = prev_t.tv_sec + 1;
			prev_t.tv_nsec = (uint32_t)(sleep_time - (s_to_ns - prev_t.tv_nsec));
		}

		do
		{
			resp = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &prev_t, NULL);
		}while(resp != 0);

		if(sch_index == 0)
		{
			Scheduler_Stamp_1 = Time_Stamp(Mode_ms);
			Monitor_Start_Stamp = Scheduler_Stamp_1;
			RGB_Start_Stamp = Scheduler_Stamp_1;
			Running_FPS_Stamp_1 = Scheduler_Stamp_1;
		}

		if((sch_cnt % Scheduler_Scaling_Factor) == 0)
		{
			sch_cnt = 0;
			if(startup >= Useless_Frames)
			{
				Monitor_Stamp_1 = Time_Stamp(Mode_ms);
				RGB_Stamp_1 = Monitor_Stamp_1;

				if(sem_post(&Monitor_Sem) != 0)
				{
					syslog(LOG_ERR, "<%.6fms>!!Scheduler!! Couldn't post Monitor_Sem", Time_Stamp(Mode_ms));
				}

				else
				{
					syslog(LOG_INFO, "<%.6fms>!!RMA!! Cam_Monitor Semaphore Posted", Time_Stamp(Mode_ms));
				}

				if(sem_post(&RGB_Sem) != 0)
				{
					syslog(LOG_ERR, "<%.6fms>!!Scheduler!! Couldn't post RGB_Sem", Time_Stamp(Mode_ms));
				}

				else
				{
					syslog(LOG_INFO, "<%.6fms>!!RMA!! Cam_RGB Semaphore Posted", Time_Stamp(Mode_ms));
				}
			}
			else
			{
				startup += 1;
			}
		}		

		if((Complete_Var & RGB_Complete_Mask) == RGB_Complete_Mask)
		{
			Complete_Var = 0;
			frames += 1;
		}

		sch_cnt += 1;

		Scheduler_Stamp_2 = Time_Stamp(Mode_ms);

		Analysis.Exec_Analysis.Scheduler_Exec[sch_index] = Scheduler_Stamp_2 - Scheduler_Stamp_1;

		sch_index += 1;

	}

	Running_FPS_Stamp_2 = Time_Stamp(Mode_ms);

	Analysis.Jitter_Analysis.Overall_Jitter[Scheduler_TID] = Running_FPS_Stamp_2 - (Running_FPS_Stamp_1 + (sch_index * Scheduler_Deadline)); 

	Analysis.Running_FPS = ((No_of_Frames + Useless_Frames) * (float)1000) / (Running_FPS_Stamp_2 - Running_FPS_Stamp_1);

	Terminate_Flag = 1;

	if(sem_post(&Monitor_Sem) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Scheduler!! Couldn't post Monitor_Sem", Time_Stamp(Mode_ms));
	}

	if(sem_post(&RGB_Sem) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Scheduler!! Couldn't post RGB_Sem", Time_Stamp(Mode_ms));
	}

	syslog(LOG_INFO, "<%.6fms>!!Scheduler!! Exiting...", Time_Stamp(Mode_ms));

	do{
		usleep(Post_Exit_Wait_ms * ms_to_us);
	} while(resp != 0);

	pthread_exit(0);
}
