#include "main.h"

extern sem_t Fib10_Sem, Fib20_Sem;
extern uint8_t Terminate_Flag;

void *Fib_Scheduler(void *threadp)
{
	uint8_t i, j;
	for(j = 0; j < 1; j ++)
	{
		for(i = 0; i < 2; i ++)
		{
			sem_post(&Fib10_Sem);
			syslog (LOG_INFO, "<%.3fms>Sem Released: Fib10", Time_Stamp());
			usleep(10000);
			sem_post(&Fib20_Sem);
			syslog (LOG_INFO, "<%.3fms>Sem Released: Fib20", Time_Stamp());
			usleep(10000);
			sem_post(&Fib10_Sem);
			syslog (LOG_INFO, "<%.3fms>Sem Released: Fib10", Time_Stamp());
			usleep(20000);
		}
		sem_post(&Fib10_Sem);
			syslog (LOG_INFO, "<%.3fms>Sem Released: Fib10", Time_Stamp());
		usleep(20000);
	}

	Terminate_Flag = 1;

	sem_post(&Fib10_Sem);
	usleep(1000);

	if(Terminate_Flag == 2)		syslog(LOG_INFO, "<%.3fms>Fib10 Exited...", Time_Stamp());
	else	syslog(LOG_ERR, "<%.3fms>Fib10 Exit Failed!", Time_Stamp());

	sem_post(&Fib20_Sem);
	usleep(1000);

	if(Terminate_Flag == 3)		syslog(LOG_INFO, "<%.3fms>Fib20 Exited...", Time_Stamp());
	else	syslog(LOG_ERR, "<%.3fms>Fib20 Exit Failed!", Time_Stamp());

	pthread_exit(0);

}
