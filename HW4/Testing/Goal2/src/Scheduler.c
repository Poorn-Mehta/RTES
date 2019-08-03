#include "main.h"

extern uint8_t Terminate_Flag;
extern uint32_t HRES, VRES;
extern uint8_t Complete_Var;	
extern sem_t Sched_Sem, Brightness_Sem, Monitor_Sem;
extern struct timespec prev_t;

static uint32_t frames = 0, sch_cnt;
static int resp;
static uint8_t startup;

void *Scheduler_Func(void *para_t)
{
	Complete_Var = 0;
	frames = 0;
	Terminate_Flag = 0;
	sch_cnt = 1;
	startup = 0;

	mq_unlink(storage_q_name);
	mq_unlink(socket_q_name);

	const uint32_t sleep_time = (Deadline_ms * ms_to_ns) / 100;

	syslog (LOG_INFO, "<%.6fms>!!Scheduler!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

//	clock_gettime(CLOCK_REALTIME, &prev_t);

	while(frames < No_of_Frames)
	{

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

/*		if((sch_cnt % 25) == 0)
		{
			sem_post(&Monitor_Sem);
		}*/

		if((sch_cnt % 100) == 0)
		{
			sch_cnt = 0;
			sem_post(&Monitor_Sem);
			if(startup >= Useless_Frames)
			{
				sem_post(&Brightness_Sem);
			}
			else
			{
				startup += 1;
			}
		}		

		if((Complete_Var & Brightness_Complete_Mask) == Brightness_Complete_Mask)
		{
			Complete_Var = 0;
			frames += 1;
		}

		sch_cnt += 1;
	}

	Terminate_Flag = 1;

	sem_post(&Brightness_Sem);
	sem_post(&Monitor_Sem);

	syslog (LOG_INFO, "<%.6fms>!!Scheduler!! Exiting...", Time_Stamp(Mode_ms));

	usleep(100000);
	pthread_exit(0);
}
