#include "main.h"

extern struct timespec start_time;
extern pthread_attr_t Attr_All;
extern struct sched_param Attr_Sch;
extern uint8_t FIFO_Max_Prio, FIFO_Min_Prio;

extern cpu_set_t CPU_Core;

float Time_Stamp(void)
{
	struct timespec curr_time = {0, 0};
	clock_gettime(CLOCK_REALTIME, &curr_time);
	float rval = 0;
	if(curr_time.tv_nsec >= start_time.tv_nsec)
		rval = ((curr_time.tv_sec - start_time.tv_sec) * 1000) + (float)((curr_time.tv_nsec - start_time.tv_nsec) / 1000000);
	else
		rval = ((curr_time.tv_sec - start_time.tv_sec - 1) * 1000) + (float)((1000000000 - (start_time.tv_nsec - curr_time.tv_nsec)) / 1000000);

	return (rval);
}

void Set_Logger(char *logname, int level_upto)
{
	setlogmask(LOG_UPTO(level_upto));
	openlog(logname, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
}

uint8_t Realtime_Setup(uint8_t Bind_to_CPU)
{

	int sched;

	if(pthread_attr_init(&Attr_All) != 0)
	{
		syslog(LOG_ERR, "Failed to Initialize Attr_All");
		return 1;
	}

	if(pthread_attr_setinheritsched(&Attr_All, PTHREAD_EXPLICIT_SCHED) != 0)
	{
		syslog(LOG_ERR, "Failed to Set Schedule Inheritance");
		return 1;
	}

	if(pthread_attr_setschedpolicy(&Attr_All, SCHED_FIFO) != 0)
	{
		syslog(LOG_ERR, "Failed to Set Scheduling Policy");
		return 1;
	}

	if(Bind_to_CPU < No_of_Cores)
	{
		CPU_ZERO(&CPU_Core);
		CPU_SET(3, &CPU_Core);
		if(pthread_attr_setaffinity_np(&Attr_All, sizeof(cpu_set_t), &CPU_Core) != 0)
		{
			syslog(LOG_ERR, "Failed to Set CPU Core");
			return 1;
		}
	}

	FIFO_Max_Prio = sched_get_priority_max(SCHED_FIFO);
	FIFO_Min_Prio = sched_get_priority_min(SCHED_FIFO);

	Attr_Sch.sched_priority = FIFO_Max_Prio;
	if(sched_setscheduler(getpid(), SCHED_FIFO, &Attr_Sch) != 0)
	{
		perror("sched_setscheduler");
		syslog(LOG_ERR, "Failed to Set FIFO Scheduler");
		return 1;
	}

	sched = sched_getscheduler(getpid());
	if(sched == SCHED_FIFO)		syslog (LOG_INFO, "Scheduling set to FIFO");
	else
	{
		syslog(LOG_ERR, "Scheduling is not FIFO");
		return 1;
	}

	return 0;

}
