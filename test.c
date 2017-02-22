
#include "mythreads.h"
#include <stdlib.h>
#include <stdio.h>
//#include <search.h>
//#include <hashtable.h>

void* t(void* arg)
{
int argi = *((int*) arg);
void* ret;

printf("%d\n", *((int*)arg));


threadYield();
argi+=10;
threadYield();
threadYield();
threadYield();
threadYield();
ret = malloc(sizeof(int));
*((int*)ret) = argi;
printf("second\n");


//threadExit(ret);
return ret;

}




int main(void)
{
threadInit();
//int a1 = 1;
//int a2 = 2;



//printf("1\n");
int arg1 = 1;
int arg2 = 100;
int res1;


int id1 = threadCreate(&t ,(void*) &arg1);
printf("start %d\n", id1);
int id2 = threadCreate(&t ,(void*) &arg2);
printf("start %d\n", id2);



printf("done\n");


//threadYield();
//threadYield();

void* ret1 = NULL;
threadJoin(id1, &ret1);
if (ret1 != NULL)
	{
	res1 = *((int*) ret1);
	free(ret1);
	printf("out %d\n", res1);
	}

threadJoin(id2, NULL);

//threadYield();

return 0;
}


