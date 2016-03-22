#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include<stdio.h>



#include "MyThread.h"


#ifndef NULL
#define NULL   ((void *) 0)
#endif

#define STACK_SIZE 160000
#define SECOND 1000

int n;
int toggle;
void* tempForStack;

int setAlarm;

struct thread
{
	   int threadID;
	   //enum State { novice,ready,running,sleeping,suspended} state;
	   double numberOfBursts;
	   double totalExecTime;
	   double totalSleepTime;
	   double totalWaitingTime;
	   void (*f)(void);
	   char stack[STACK_SIZE];
	   jmp_buf jbuf;
};

struct queueNode
{
	struct thread  *thread_pointer;
	struct queueNode *nextNode;
};

struct thread *currentExecutingThread;


struct queueNode *start_of_novice=NULL;
struct queueNode *end_of_novice=NULL;

struct queueNode *start_of_ready=NULL;
struct queueNode *end_of_ready=NULL;

struct queueNode *start_of_suspended=NULL;
struct queueNode *end_of_suspended=NULL;

struct queueNode *start_of_running=NULL;
struct queueNode *end_of_running=NULL;

struct queueNode *start_of_sleeping=NULL;
struct queueNode *end_of_sleeping=NULL;


struct thread* removeThreadFromQueue(struct queueNode **start, struct queueNode **end)
{
	if(*start==NULL)
	{
		return NULL;
	}
	else
	{
		struct queueNode* temp = *start;

		if(*start==*end)
		{
			*start = *end =NULL;
		}
		else
		{
			*start= (*start)->nextNode;
		}
		temp->nextNode=NULL;
		return temp->thread_pointer;
	}
}

void insertThreadIntoQueue(struct queueNode **start,struct queueNode **end, struct thread *threadTobeInserted)
{
	struct queueNode *temp = (struct queueNode * )malloc(sizeof(struct queueNode));

	temp->thread_pointer=threadTobeInserted;
	temp->nextNode =NULL;


	if(*start ==NULL)
	{
		*start=*end=temp;
	}
	else
	{
		(*end) -> nextNode = temp;
		*end = temp;
	}
}

struct thread* removeThreadFromQueueUsingThreadID(struct queueNode** start,struct queueNode** end , int thread_id)
{
	if(*start==NULL)
	{
		return NULL;
	}
	else
	{
		struct queueNode *temp=*start;
		struct queueNode *prevThread=NULL;

		while(temp!=NULL)
		{
		   if(temp->thread_pointer->threadID == thread_id)
		   {
			   if(prevThread==NULL)
			   {
				   if(temp==*end)
				   {
					    *start=*end=NULL;
				   }
				   else
				   {
					   *start=temp-> nextNode;
				   }
			   }
			   else
			   {
				   prevThread->nextNode=temp->nextNode;
			   }

			   temp->nextNode=NULL;

			   return temp->thread_pointer;

			   break;
		   }
		   prevThread=temp;
		   temp=temp->nextNode;
		}
		return NULL;
	}
}


int create(void (*fun)(void))
{
		struct thread *t = (struct thread * )malloc(sizeof(struct thread));
		t->threadID=n;
		n++;
		//t->state=novice;
		t->numberOfBursts=0;
		t->totalExecTime=0;
		t->totalSleepTime=0;
		t->totalWaitingTime=0;
		t->f = fun;

		struct queueNode *temp = (struct queueNode * )malloc(sizeof(struct queueNode));

		temp->thread_pointer=t;
		temp->nextNode =NULL;

		if(start_of_novice == NULL)
		{
			start_of_novice = end_of_novice = temp;
		}
		else
		{
			end_of_novice->nextNode = temp;
			end_of_novice = temp;
		}
		return t->threadID;
}

void dispatch(int a)
{
	printf("Inside Hello");
	insertThreadIntoQueue(&start_of_ready,&end_of_ready,currentExecutingThread);
	if(sigsetjmp(currentExecutingThread->jbuf,1)==1)
	{
		return;
	}

	toggle = (toggle + 1 )% n;

	currentExecutingThread = removeThreadFromQueue(&start_of_ready,&end_of_ready);

	siglongjmp(currentExecutingThread ->jbuf,1);
}

void start()
{
	struct thread *t;

	struct sigaction act;
	act.sa_handler = dispatch;
	sigaction (SIGALRM, & act, 0);

	while(start_of_novice != NULL)
	{
		t=removeThreadFromQueue(&start_of_novice,&end_of_novice);
		insertThreadIntoQueue(&start_of_ready,&end_of_ready,t);

		if(sigsetjmp(t->jbuf,1))
		{
			if(!setAlarm)
			{
				printf("Alarm is set\n");
				setAlarm=1;
				ualarm(SECOND,SECOND);
			}
		    tempForStack = currentExecutingThread -> stack + STACK_SIZE;
		    __asm__ __volatile__("movl tempForStack,%esp");
		    (*currentExecutingThread->f)();
		}
	}

	currentExecutingThread = removeThreadFromQueue(&start_of_ready,&end_of_ready);
    longjmp(currentExecutingThread->jbuf,1);
}

void suspend(int threadIDToBeSuspended)
{
	struct thread *temp;
	temp=removeThreadFromQueueUsingThreadID(&start_of_ready,&end_of_ready,threadIDToBeSuspended);
	if(temp==NULL)
	{
		printf("Thread does not exist");
		return;
	}
	else
	{
		insertThreadIntoQueue(&start_of_suspended,&end_of_suspended,temp);
	}
}

void resume(int threadIDToBeResumed)
{
	struct thread *temp;
	temp=removeThreadFromQueueUsingThreadID(&start_of_suspended,&end_of_suspended,threadIDToBeResumed);
	if(temp==NULL)
	{
		printf("Thread does not exist");
		return;
	}
	else
	{
		insertThreadIntoQueue(&start_of_ready,&end_of_ready,temp);
	}
}
void delete(int threadID)
{
	struct thread *temp=NULL;
	temp=removeThreadFromQueueUsingThreadID(&start_of_novice,&end_of_novice,threadID);
	if(temp!=NULL)
	{
		printf("Thread Deleted Successfully from Novice Queue");
		return;
	}
	temp=removeThreadFromQueueUsingThreadID(&start_of_running,&end_of_running,threadID);
	if(temp!=NULL)
	{
		printf("Thread Deleted Successfully from Running Queue");
		return;
	}
	temp=removeThreadFromQueueUsingThreadID(&start_of_ready,&end_of_ready,threadID);
	if(temp!=NULL)
	{
		printf("Thread Deleted Successfully from Ready Queue");
		return;
	}
	temp=removeThreadFromQueueUsingThreadID(&start_of_suspended,&end_of_suspended,threadID);
	if(temp!=NULL)
	{
		printf("Thread Deleted Successfully from Suspended Queue");
		return;
	}
	temp=removeThreadFromQueueUsingThreadID(&start_of_sleeping,&end_of_sleeping,threadID);
	if(temp!=NULL)
	{
		printf("Thread Deleted Successfully from Sleeping Queue");
		return;
	}

}

