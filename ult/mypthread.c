//#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include "mypthread.h"

#define STACKSIZE 1024*8
#define MAXTHREADS 512

int count = 0;								//number of active threads
int threadIndex = 0;						//index of the current running thread
int mainSetup = 0;
static mypthread_t allThreads[MAXTHREADS];	//array of all the threads

//private function prototypes
void initThreads(void);
int getUnused(void);
int getReady(void);
int getWaiting(void);
/////////////////////////////

int mypthread_create(mypthread_t *thread, const mypthread_attr_t *attr,
			void *(*start_routine) (void *), void *arg) 
{	
	if (count == MAXTHREADS)
	{
		printf("Maximum number of threads reached: %d\n", MAXTHREADS);
		return -1;
	}
	static int firstRun = 1;	
	if (firstRun)
	{
		initThreads();	
		firstRun = 0;	
		mainSetup = 0;
	}
	int unused = getUnused();
	if (getcontext(&(allThreads[unused].context)) != 0) {
		perror("Error getting context.\n");
		return -1;
	}
	allThreads[unused].context.uc_stack.ss_sp = malloc(STACKSIZE);
	allThreads[unused].context.uc_stack.ss_size = STACKSIZE;
	
	makecontext(&(allThreads[unused].context), (void*) start_routine, 1, arg);
	
	allThreads[unused].status = READY;
	*thread = allThreads[unused];
	count++;

	return 0;
}

void mypthread_exit(void *retval) 
{

	allThreads[threadIndex].status = EXITED;
	allThreads[threadIndex].retval = retval;
	free(allThreads[threadIndex].context.uc_stack.ss_sp);

	if(allThreads[threadIndex].retadd)
	{
		*(allThreads[threadIndex].retadd) = retval;
	}

	if (allThreads[threadIndex].beingJoinedOnBy > -1)
	{
		int joiningThreadIndex = allThreads[threadIndex].beingJoinedOnBy;
		allThreads[joiningThreadIndex].status = READY;
	}

	int next = getReady();
	if (next < 0)
	{
		int waiting = getWaiting();
		if (waiting < 0)
		{
			printf("No waiting or exiting threads found. Exiting.\n");
			exit(1);
		}
		next = allThreads[waiting].index;
	}
	threadIndex = next;

	allThreads[threadIndex].status = RUNNING;
	setcontext(&(allThreads[threadIndex].context));
	
}

int mypthread_join(mypthread_t thread, void **retval) 
{
	if (!mainSetup)
	{
		int unused = getUnused();
		if (getcontext(&(allThreads[unused].context)) != 0)
		{
			printf("Error getting context.\n");
			exit(0);
		}
		allThreads[unused].context.uc_stack.ss_sp = malloc(STACKSIZE);
		allThreads[unused].context.uc_stack.ss_size = STACKSIZE;

		allThreads[thread.index].beingJoinedOnBy = allThreads[unused].index;

		threadIndex = thread.index;

		allThreads[threadIndex].status = RUNNING;
		allThreads[unused].status = WAITING;

		if (retval)
		{	
			printf("retval detected in join call, setting return address in thread struct\n");
			allThreads[threadIndex].retadd = retval;
			printf("retadd set to %p\n", allThreads[threadIndex].retadd);
		}

		mainSetup = 1;

		swapcontext(&(allThreads[unused].context), &(allThreads[threadIndex].context));

		return 0;
	}
	if (threadIndex == thread.index)
	{
		printf("A thread cannot join on itself.\n");
		return -1;
	}
	if (allThreads[thread.index].beingJoinedOnBy > -1)
	{
		printf("Thread is already being joined on.\n");
		return -1;
	}
	if (retval)
		{	
			printf("retval detected in join call (not in main setup), setting return address in thread struct\n");
			allThreads[thread.index].retadd = retval;
			printf("retadd set to %p\n", allThreads[threadIndex].retadd);
		}
	if (allThreads[threadIndex].status == EXITED)
	{
		allThreads[threadIndex].status = UNUSED;
		free(allThreads[threadIndex].context.uc_stack.ss_sp);
		allThreads[threadIndex].beingJoinedOnBy = -1;
		count--;
		return 0;
	}

	allThreads[threadIndex].status = WAITING;
	allThreads[thread.index].beingJoinedOnBy = threadIndex;

	int next = getReady();

	if (next < 0)
	{
		int waiting = getWaiting();
		
		if (waiting < 0)
		{
			printf("No waiting threads found.\n");
			exit(0);
		}	
		next = allThreads[waiting].index;
	}

	int old = threadIndex;
	threadIndex = next;
	allThreads[threadIndex].status = RUNNING;
	swapcontext(&(allThreads[old].context), &(allThreads[threadIndex].context));
	return 0;
}

int mypthread_yield(void) 
{
	if (!mainSetup)
	{
		int unused = getUnused();
		if (getcontext(&(allThreads[unused].context)) != 0)
		{
			printf("Error getting context.\n");
			exit(0);
		}

		allThreads[unused].context.uc_stack.ss_sp = malloc(STACKSIZE);
		allThreads[unused].context.uc_stack.ss_size = STACKSIZE;
		allThreads[unused].status = READY;
		allThreads[threadIndex].status = RUNNING;

		mainSetup = 1;

		swapcontext(&(allThreads[unused].context), &(allThreads[threadIndex].context));
		return 0;
	}
	allThreads[threadIndex].status = READY;

	int next = getReady();

	if (next < 0)
	{
		int waiting = getWaiting();
		
		if (waiting < 0)
		{
			printf("No waiting threads found.\n");
			exit(0);
		}
		
		next = allThreads[waiting].index;
	}

	int old = threadIndex;

	threadIndex = next;

	allThreads[threadIndex].status = RUNNING;

	swapcontext(&(allThreads[old].context), &(allThreads[threadIndex].context));
	
	return 0;
}

/////////////////////
/*private functions*/
/////////////////////
void initThreads(void) 
{
	int i = 0;
	mainSetup = 0;
	
	for (; i < MAXTHREADS; i++)
	{
		allThreads[i].index = i;
		allThreads[i].status = UNUSED;
		allThreads[i].beingJoinedOnBy = -1;
		allThreads[i].retval = NULL;
		allThreads[i].retadd = NULL;
	}
}
int getUnused(void) 
{
	int i = threadIndex;
	int startingPoint = threadIndex;
	
	for (; i < MAXTHREADS; i++) 
	{
		if (allThreads[i].status == UNUSED) 
		{	
			return i;
		}
	}
	
	i = 0;
	
	for (; i <= startingPoint; i++)
	{
		if (allThreads[i].status == UNUSED) 
		{
			return i;
		}
	}
	return -1;
}
int getReady(void) 
{
	int i = threadIndex+1;
	int startingPoint = threadIndex;
	
	for (; i < MAXTHREADS; i++) 
	{
		if (allThreads[i].status == READY) 
		{	
			return i;
		}
	}
	
	i = 0;

	for (; i <= startingPoint; i++)
	{
		if (allThreads[i].status == READY) 
		{
			return i;
		}
	}
	return -1;
}
int getWaiting(void)
{

	int i = 0;
	
	for ( ; i<MAXTHREADS; i++)
	{
		if (allThreads[i].status == WAITING)
		{
			return i;
		}
	}
	return -1;
}
