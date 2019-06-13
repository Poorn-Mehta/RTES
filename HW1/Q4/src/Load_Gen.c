#include "main.h"

void Fib_Calc(uint32_t loops)
{
	uint32_t b = 1, a = 0, fib_num, i, j;
	for(j = 0; j < loops; j ++)
	{
		for(i = 0; i < 45; i ++)
		{
			fib_num = a + b;
			a = b;
			b = fib_num;
		}
		a = 0;
		b = 1;
	}
}

void Dly_ms(uint8_t req_ms)
{
	uint32_t b = 1, a = 0, fib_num, i, j = 0;
	struct timespec Temp1 = {0, 0};
	struct timespec Temp2 = {0, 0};
	clock_gettime(CLOCK_REALTIME, &Temp1);
	while(j < (req_ms * 1000))
	{
		for(i = 0; i < 45; i ++)
		{
			fib_num = a + b;
			a = b;
			b = fib_num;
		}
		a = 0;
		b = 1;
		clock_gettime(CLOCK_REALTIME, &Temp2);
		j = (Temp2.tv_nsec - Temp1.tv_nsec) / 1000;
	}
}
