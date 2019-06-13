#include "main.h"

struct timespec start_time = {0, 0}; 

float Fib10_Process_Timems = 0;
float Fib20_Process_Timems = 0;

uint8_t Terminate_Flag = 0;


pthread_t Fib10, Fib20, Fib_Sch;
sem_t Fib10_Sem, Fib20_Sem;

pthread_attr_t Attr_All;
struct sched_param Attr_Sch;
uint8_t FIFO_Max_Prio = 99, FIFO_Min_Prio = 1;

uint32_t Fib_Loops_for_10ms, Fib_Loops_for_20ms;
cpu_set_t CPU_Core;

int main (int argc, char *argv[])
{
	clock_gettime(CLOCK_REALTIME, &start_time);

	Set_Logger("HW1_Q4_Log", LOG_DEBUG);

	if(Realtime_Setup(3) != 0)	exit(-1);

	if(sem_init(&Fib10_Sem, 0, 0) != 0)
	{
	    syslog(LOG_ERR, "Failed to initialize Fib10 semaphore");
	    exit(-1);
	}

	if(sem_init(&Fib20_Sem, 0, 0) != 0)
	{
	    syslog(LOG_ERR, "Failed to initialize Fib20 semaphore");
	    exit(-1);
	}

	Attr_Sch.sched_priority = FIFO_Max_Prio;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Fib_Sch, &Attr_All, Fib_Scheduler, (void *)0);

	Attr_Sch.sched_priority = FIFO_Max_Prio - 1;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Fib10, &Attr_All, Fib10_Func, (void *)0);

	Attr_Sch.sched_priority = FIFO_Max_Prio - 2;
	pthread_attr_setschedparam(&Attr_All, &Attr_Sch);
	pthread_create(&Fib20, &Attr_All, Fib20_Func, (void *)0); 

	pthread_join(Fib10, 0);
	pthread_join(Fib20, 0);
	pthread_join(Fib_Sch, 0);

	syslog (LOG_INFO, ">>>>>>>>>> Program End <<<<<<<<<<");

	closelog();

	pthread_attr_destroy(&Attr_All);

	sem_destroy(&Fib10_Sem);
	sem_destroy(&Fib20_Sem);

	exit(0);
}
