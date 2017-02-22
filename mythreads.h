/*---- FILE HEADER ----------------------------------------------------------
project: project2
file: mythreads.h
author: elias abderhalden
date: 2014-10-17
-----------------------------------------------------------------------------
class: ece3220 fall 2014
instructor: jacob sorber
assignment: project #2
purpose: User Mode Thread Library
---------------------------------------------------------------------------*/


//#include <ucontext.h>
//#include "lib"

#define STACK_SIZE (16*1024)
#define NUM_LOCKS 10
#define CONDITIONS_PER_LOCK 10


typedef void* (*ThreadFuncPtr)(void *);


extern void threadInit();
extern int threadCreate(ThreadFuncPtr function_ptr, void* argument_ptr);
extern void threadYield();
extern void threadJoin(int thread_id, void **returnval);
extern void threadExit(void* returnval);


extern void threadLock(int locknum);
extern void threadUnlock(int locknum);
extern void threadWait(int locknum, int conditionnum);
extern void threadSignal(int locknum, int conditionnum);



//extern int currentid;

extern int interruptsAreDisabled;



