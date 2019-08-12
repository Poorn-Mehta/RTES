#include <math.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define U32_T unsigned int

U32_T ex0_period[] = {10, 1000, 1000};
U32_T ex0_wcet[] = {1, 1, 195};

int completion_time_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);
int scheduling_point_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);


int main(void)
{
    int i;
	U32_T numServices;

    printf("******** Completion Test Feasibility Example\n");

    printf("Ex-0 U=%4.2f (C1=1, C2=1, C3=195; T1=10, T2=1000, T3=1000; T=D): ",
		   ((1.0/10.0) + (1.0/1000.0) + (195.0/1000.0)));
	numServices = sizeof(ex0_period)/sizeof(U32_T);
    if(completion_time_feasibility(numServices, ex0_period, ex0_wcet, ex0_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");

	printf("\n\n");
    printf("******** Scheduling Point Feasibility Example\n");

    printf("Ex-0 U=%4.2f (C1=1, C2=1, C3=195; T1=10, T2=1000, T3=1000; T=D): ",
		   ((1.0/10.0) + (1.0/1000.0) + (195.0/1000.0)));
	numServices = sizeof(ex0_period)/sizeof(U32_T);
    if(scheduling_point_feasibility(numServices, ex0_period, ex0_wcet, ex0_period) == TRUE)
        printf("FEASIBLE\n");
    else
        printf("INFEASIBLE\n");

}


int completion_time_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[])
{
  int i, j;
  U32_T an, anext;

  // assume feasible until we find otherwise
  int set_feasible=TRUE;

  //printf("numServices=%d\n", numServices);

  for (i=0; i < numServices; i++)
  {
       an=0; anext=0;

       for (j=0; j <= i; j++)
       {
           an+=wcet[j];
       }

	   //printf("i=%d, an=%d\n", i, an);

       while(1)
       {
             anext=wcet[i];

             for (j=0; j < i; j++)
                 anext += ceil(((double)an)/((double)period[j]))*wcet[j];

             if (anext == an)
                break;
             else
                an=anext;

			 //printf("an=%d, anext=%d\n", an, anext);
       }

	   //printf("an=%d, deadline[%d]=%d\n", an, i, deadline[i]);

       if (an > deadline[i])
       {
          set_feasible=FALSE;
       }
  }

  return set_feasible;
}


int scheduling_point_feasibility(U32_T numServices, U32_T period[],
								 U32_T wcet[], U32_T deadline[])
{
   int rc = TRUE, i, j, k, l, status, temp;

   for (i=0; i < numServices; i++) // iterate from highest to lowest priority
   {
      status=0;

      for (k=0; k<=i; k++)
      {
          for (l=1; l <= (floor((double)period[i]/(double)period[k])); l++)
          {
               temp=0;

               for (j=0; j<=i; j++) temp += wcet[j] * ceil((double)l*(double)period[k]/(double)period[j]);

               if (temp <= (l*period[k]))
			   {
				   status=1;
				   break;
			   }
           }
           if (status) break;
      }
      if (!status) rc=FALSE;
   }
   return rc;
}
