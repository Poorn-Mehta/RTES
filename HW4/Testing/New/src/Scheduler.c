#include "main.h"

extern uint8_t Terminate_Flag, Operating_Mode;
extern uint32_t HRES, VRES, Scheduler_Deadline, Deadline_ms, sch_index;
extern uint8_t Complete_Var;	
extern sem_t Sched_Sem, Brightness_Sem, Monitor_Sem;
extern struct timespec prev_t;
extern strct_analyze Analysis;

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

	mq_unlink(storage_q_name);
	mq_unlink(socket_q_name);

	const uint32_t sleep_time = (Deadline_ms * ms_to_ns) / 100;

	syslog (LOG_INFO, "<%.6fms>!!Scheduler!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	if((Operating_Mode & Mode_FPS_Mask) == Mode_10_FPS_Val)
	{
		clock_gettime(CLOCK_REALTIME, &prev_t);
	}

	Running_FPS_Stamp_1 = Time_Stamp(Mode_ms);

	while(frames < No_of_Frames)
	{

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
//			Deadline_Stamp_1 = Scheduler_Stamp_1;
		}

		if((sch_cnt % 10) == 0)
		{
			sem_post(&Monitor_Sem);
		}

		if((sch_cnt % 100) == 0)
		{
			sch_cnt = 0;
//			sem_post(&Monitor_Sem);
			if(startup >= Useless_Frames)
			{
//				Deadline_Stamp_1 = Time_Stamp(Mode_ms);
				sem_post(&Brightness_Sem);
			}
			else
			{
				startup += 1;
			}
		}		

		if((Complete_Var & Brightness_Complete_Mask) == Brightness_Complete_Mask)
		{
/*			Deadline_Stamp_2 = Time_Stamp(Mode_ms);

			if((Deadline_Stamp_2 - Deadline_Stamp_1) > (float)Deadline_ms)
			{
				Analysis.Missed_Deadlines += 1;
				syslog (LOG_INFO, "!!WEIRD!! Diff: %.3f", Deadline_Stamp_2 - Deadline_Stamp_1);
			}*/

			Complete_Var = 0;
			frames += 1;

//			Deadline_Stamp_1 = Time_Stamp(Mode_ms);
		}

		sch_cnt += 1;

		Scheduler_Stamp_2 = Time_Stamp(Mode_ms);

		Analysis.Exec_Analysis.Scheduler_Exec[sch_index] = Scheduler_Stamp_2 - Scheduler_Stamp_1;

		sch_index += 1;

	}

	Running_FPS_Stamp_2 = Time_Stamp(Mode_ms);

	Analysis.Jitter_Analysis.Overall_Jitter[Scheduler_TID] = Running_FPS_Stamp_2 - (Running_FPS_Stamp_1 + (Scheduler_Loop_Count * Scheduler_Deadline)); 

	Analysis.Running_FPS = ((No_of_Frames + Useless_Frames) * (float)1000) / (Running_FPS_Stamp_2 - Running_FPS_Stamp_1);

	Terminate_Flag = 1;

	sem_post(&Brightness_Sem);
	sem_post(&Monitor_Sem);

	syslog (LOG_INFO, "<%.6fms>!!Scheduler!! Exiting...", Time_Stamp(Mode_ms));

	usleep(100000);
	pthread_exit(0);
}
