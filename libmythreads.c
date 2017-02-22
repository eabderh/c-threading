/*---- FILE HEADER ----------------------------------------------------------
project: project2
file: libmythreads.c  
author: elias abderhalden
date: 2014-10-17
-----------------------------------------------------------------------------
class: ece3220 fall 2014
instructor: jacob sorber
assignment: project #2
purpose: User Mode Thread Library
---------------------------------------------------------------------------*/



#include <sys/queue.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ucontext.h>

#include "mythreads.h"


typedef struct ThreadInfo_t
	{
	int id;
	int calledexit;
	int waitingon;
	void* returnval;
	ucontext_t* context_ptr;
	ucontext_t* exitcontext_ptr;
	} ThreadInfo;



#define WAIT 1
#define QUEUE 0
#define LOCKED 1
#define UNLOCKED 0
#define SIG 1
#define UNSIG 0





TAILQ_HEAD(QueueHead, element);


struct element
	{
	ThreadInfo* ti_entry;
	TAILQ_ENTRY(element) entries;
	};




typedef int (*QueueCompFptr)(ThreadInfo*, ThreadInfo*);

int queue_compwait(ThreadInfo* a, ThreadInfo* b);
int queue_compid(ThreadInfo* a, ThreadInfo* b);
int queue_compid();

void queue_inserthead(struct QueueHead* tq, ThreadInfo* ti_insert);
void queue_inserttail(struct QueueHead* tq, ThreadInfo* ti_insert);
ThreadInfo* queue_head(struct QueueHead* tq);
ThreadInfo* queue_removehead(struct QueueHead* tq);
ThreadInfo* queue_remove(struct QueueHead* tq, QueueCompFptr compfunc, ThreadInfo* compdata);
ThreadInfo* queue_access(struct QueueHead* tq, QueueCompFptr compfunc, ThreadInfo* compdata);
struct element* queue_find(struct QueueHead* tq, QueueCompFptr compfunc, ThreadInfo* compdata);




void myswap(int wait_sig);
void myfuncstart( ThreadFuncPtr function_ptr, void* argument_ptr);
void myexit();
ThreadInfo* mynewinfo();

static void interruptDisable();
static void interruptEnable();


static ThreadInfo maininfo;
static ThreadInfo* currentinfo;
static ucontext_t maincontext;
static ucontext_t returncontext;
int currentid;
int interruptsAreDisabled;


static struct QueueHead threadqueue;
static struct QueueHead waitqueue;
static struct QueueHead exitqueue;
static struct QueueHead locksqueue;

int mylocks[NUM_LOCKS];
int myconditions[CONDITIONS_PER_LOCK];




void threadInit()
{

TAILQ_INIT(&threadqueue);
TAILQ_INIT(&waitqueue);
TAILQ_INIT(&exitqueue);
TAILQ_INIT(&locksqueue);


interruptsAreDisabled = 0;

currentid = 0;

currentinfo = &maininfo;
currentinfo->context_ptr = &maincontext;
currentinfo->id = currentid;
currentid++;

return;
}




int threadCreate(ThreadFuncPtr function_ptr, void* argument_ptr)
{
int newid;
ThreadInfo* newinfo;
ucontext_t* newcontext;

interruptDisable();

newinfo = mynewinfo();
newcontext = newinfo->context_ptr;
newid = newinfo->id;
makecontext(newcontext, (void(*) (void)) myfuncstart, 2, function_ptr, argument_ptr);

queue_inserthead(&threadqueue, newinfo);

myswap(QUEUE);

interruptEnable();
return newid;
}




void threadYield()
{
interruptDisable();

myswap(QUEUE);

interruptEnable();
return;
}



void threadJoin(int id, void** returnval_ptr)
{
interruptDisable();

ThreadInfo* accessinfo;
ThreadInfo compinfo;

compinfo.id = id;
accessinfo = queue_access(&threadqueue, &queue_compid, &compinfo);
if (accessinfo == NULL)
	{

	accessinfo = queue_access(&waitqueue, &queue_compid, &compinfo);
	if (accessinfo == NULL)
		{
		if (returnval_ptr != NULL)
			returnval_ptr = NULL;
		interruptEnable();
		return;
		}
	}

currentinfo->waitingon = id;
queue_inserthead(&waitqueue, currentinfo);

myswap(WAIT);

if (returnval_ptr != NULL)
	*returnval_ptr = currentinfo->returnval;

interruptEnable();
return;
}




void threadExit(void* returnval)
{
ThreadInfo* waitinfo;

interruptDisable();

while (1)
	{
	waitinfo = queue_remove(&waitqueue, &queue_compwait, currentinfo);
	if (waitinfo == NULL)
		break;
	waitinfo->returnval = returnval;
	queue_inserthead(&threadqueue, waitinfo);
	}
currentinfo->calledexit = 1;

setcontext(currentinfo->exitcontext_ptr);
}



int mylocks[NUM_LOCKS];
int myconditions[CONDITIONS_PER_LOCK];








void threadLock(int locknum)
{
interruptDisable();


while (mylocks[locknum] == LOCKED);
	{
	myswap(QUEUE);
	}

mylocks[locknum] = LOCKED;

interruptEnable();
return;
}




void threadUnlock(int locknum)
{
interruptDisable();

mylocks[locknum] = UNLOCKED;

interruptEnable();
return;
}




void threadWait(int locknum, int conditionnum)
{
interruptDisable();

if (mylocks[locknum] != LOCKED)
	{
	fprintf(stderr, "threadWait: Unlocked mutex\n");
	exit(2);
	}
mylocks[locknum] = UNLOCKED;

while (~((mylocks[locknum] == UNLOCKED) && (myconditions[conditionnum] == SIG)))
	{
	myswap(QUEUE);
	}

mylocks[locknum] = LOCKED;
myconditions[conditionnum] = UNSIG;

interruptEnable();
return;
}




void threadSignal(int locknum, int conditionnum)
{
interruptDisable();

myconditions[conditionnum] = SIG;

interruptEnable();
return;
}



















void myswap(int wait_sig)
{

ThreadInfo* oldinfo;
ThreadInfo* newinfo;
ucontext_t* oldcontext;
ucontext_t* newcontext;

newinfo = queue_removehead(&threadqueue);
if (newinfo == NULL)
	{
	return;
	}
newcontext = newinfo->context_ptr;


oldinfo = currentinfo;
oldcontext = oldinfo->context_ptr;

if (!wait_sig)
	{
	queue_inserttail(&threadqueue, oldinfo);
	}
currentinfo = newinfo;

swapcontext( oldcontext, newcontext);

return;
}




void myfuncstart( ThreadFuncPtr function_ptr, void* argument_ptr)
{
void* returnval = NULL;


ucontext_t* exitcontext;

exitcontext = currentinfo->exitcontext_ptr;
getcontext(exitcontext);

if (!(currentinfo->calledexit))
	{
	interruptEnable();
	returnval = function_ptr(argument_ptr);
	threadExit(returnval);
	}

myexit();

return;
}




void myexit()
{
ThreadInfo* nextinfo;
ucontext_t* nextcontext;
char* stack;


stack = currentinfo->context_ptr->uc_stack.ss_sp;

queue_inserttail(&exitqueue, currentinfo);

nextinfo = queue_removehead(&threadqueue);
currentinfo = nextinfo;
nextcontext = nextinfo->context_ptr;
returncontext = *nextcontext;


free(stack);


return;
}




ThreadInfo* mynewinfo()
{

ThreadInfo* newinfo;
ucontext_t* newcontext;
ucontext_t* exitcontext;
int newid;
char* stack;


newinfo = queue_removehead(&exitqueue);
if (newinfo == NULL)
	{
	newinfo = (ThreadInfo*) malloc(sizeof(ThreadInfo));
	if (newinfo == NULL)
		{
		perror("malloc");
		exit(1);
		}
	newcontext = (ucontext_t*) malloc(sizeof(ucontext_t));
	if (newinfo == NULL)
		{
		perror("malloc");
		exit(1);
		}
	exitcontext = (ucontext_t*) malloc(sizeof(ucontext_t));
	if (newinfo == NULL)
		{
		perror("malloc");
		exit(1);
		}
	 
	newid = currentid;
	currentid++;
	}
else
	{
	newcontext = newinfo->context_ptr;
	exitcontext = newinfo->exitcontext_ptr;
	newid = newinfo->id;
	}



getcontext(newcontext);
getcontext(&returncontext);

stack = malloc(STACK_SIZE);
if (stack == NULL)
	{
	perror("malloc");
	exit(1);
	}

newcontext->uc_stack.ss_sp = stack;
newcontext->uc_stack.ss_size = STACK_SIZE;
newcontext->uc_stack.ss_flags = 0;
newcontext->uc_link = &returncontext;


newinfo->context_ptr = newcontext;
newinfo->exitcontext_ptr = exitcontext;
newinfo->calledexit = 0;
newinfo->id = newid;

return newinfo;
}











void interruptDisable()
{
assert(!interruptsAreDisabled);
interruptsAreDisabled = 1;
}

void interruptEnable()
{
assert(interruptsAreDisabled);
interruptsAreDisabled = 0;
}














int queue_compwait(ThreadInfo* a, ThreadInfo* b)
{
if (a->waitingon == b->id)
	return 0;
if (a->waitingon < b->id)
	return 1;
return -1;
}

int queue_compid(ThreadInfo* a, ThreadInfo* b)
{
if (a->id == b->id)
	return 0;
if (a->id < b->id)
	return 1;
return -1;
}




void queue_inserthead(struct QueueHead* tq, ThreadInfo* ti_insert)
{
struct element* element_insert;
element_insert = (struct element*) malloc(sizeof(struct element*));
if (element_insert == NULL)
	{
	perror("malloc");
	exit(1);
	}
element_insert->ti_entry = ti_insert;

TAILQ_INSERT_HEAD(tq, element_insert, entries);

return;
}




void queue_inserttail(struct QueueHead* tq, ThreadInfo* ti_insert)
{
struct element* element_insert;
element_insert = (struct element*) malloc(sizeof(struct element*));
if (element_insert == NULL)
	{
	perror("malloc");
	exit(1);
	}
element_insert->ti_entry = ti_insert;

TAILQ_INSERT_TAIL(tq, element_insert, entries);

return;
}




ThreadInfo* queue_head(struct QueueHead* tq)
{
struct element* element_access;
ThreadInfo* ti_access;

element_access = tq->tqh_first;
if (element_access == NULL)
	return NULL;

ti_access = element_access->ti_entry;

return ti_access;
}




ThreadInfo* queue_removehead(struct QueueHead* tq)
{
struct element* element_remove;
ThreadInfo* ti_remove;

element_remove = tq->tqh_first;
if (element_remove == NULL)
	return NULL;

TAILQ_REMOVE(tq, tq->tqh_first, entries);

ti_remove = element_remove->ti_entry;
free(element_remove);

return ti_remove;
}




struct element* queue_find(struct QueueHead* tq, QueueCompFptr compfunc, ThreadInfo* compdata)
{
struct element* rover;

rover = (struct element*) tq->tqh_first;

while (rover != NULL)
	{
	if (compfunc(rover->ti_entry, compdata) == 0)
		return rover;
	rover = rover->entries.tqe_next;
	}
return NULL;
}







ThreadInfo* queue_access(struct QueueHead* tq, QueueCompFptr compfunc, ThreadInfo* compdata)
{
struct element* element_find;
element_find = queue_find(tq, compfunc, compdata);
if (element_find == NULL)
	return NULL;
return element_find->ti_entry;
}





ThreadInfo* queue_remove(struct QueueHead* tq, QueueCompFptr compfunc, ThreadInfo* compdata)
{
struct element* element_find;
ThreadInfo* entry_return;

element_find = queue_find(tq, compfunc, compdata);
if (element_find == NULL)
	return NULL;
entry_return = element_find->ti_entry;
TAILQ_REMOVE(tq, element_find, entries);

return entry_return;
}







