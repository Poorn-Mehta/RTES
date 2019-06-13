#include "main.h"

extern sem_t Fib10_Sem;
extern uint8_t Terminate_Flag;
extern uint32_t Fib_Loops_for_10ms;

void *Fib10_Func(void *threadp)
{
	while(1)
	{

		sem_wait(&Fib10_Sem);

		if(Terminate_Flag != 0)		break;

		uint64_t temp_time;
		struct timespec Temp1 = {0, 0};
		struct timespec Temp2 = {0, 0};

		do
		{
			clock_gettime(CLOCK_REALTIME, &Temp1);
			Fib_Calc(1000);
			clock_gettime(CLOCK_REALTIME, &Temp2);
		}while(Temp2.tv_nsec < Temp1.tv_nsec);
		temp_time = (Temp2.tv_nsec - Temp1.tv_nsec) / 1000;

		Fib_Loops_for_10ms = (uint32_t)((uint64_t)9500000 / (uint64_t)temp_time);

		syslog (LOG_INFO, "<%.3fms>Fib10 Started - Core(%d)", Time_Stamp(), sched_getcpu());

		Fib_Calc(Fib_Loops_for_10ms);

		syslog (LOG_INFO, "<%.3fms>Fib10 Completed - Core(%d)", Time_Stamp(), sched_getcpu());
	}

	syslog(LOG_INFO, "<%.3fms>Fib10 Exiting...", Time_Stamp());
	Terminate_Flag += 1;
	pthread_exit(0);
}
