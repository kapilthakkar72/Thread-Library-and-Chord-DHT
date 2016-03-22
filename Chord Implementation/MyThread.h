#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <time.h>

#include <list>
using namespace std;

void intermediate();
void sleep(int);
void yield();
void createStats(struct thread *temp,struct statistics *stat);


#ifndef NULL
#define NULL   ((void *) 0)
#endif


/**
Stack Size of a thread
*/

#define STACK_SIZE 4096*20

/**
After this much time alarm will be called
*/
#define SECOND 100000


int n;
int toggle;
void* tempForStack;
int kt;

int setAlarm;

struct timeval startExecutionTime;
struct timeval endExecutionTime;

/**
This will maintain the current state of thread
*/

enum State { novice,ready,running,sleeping,suspended,terminated};


/**
Thread Structure to store all information reagrding thread
*/
struct thread
{
       int threadID;   /** ID for each Thread */
       enum State state;    /** To maintain state */
       double numberOfBursts;   /** How many times thread got executed */
       double totalExecTime;    /** Total time for which thread executed*/
       double totalSleepTime;   /** Total time for which thread was sleeping */
       double totalWaitingTime;     /** Total time for which thread waited in queue */
       int funType;     /** To distinguish between type of thread(with and without argument) */
       void *arg;       /** Argument to function if any */
       void *(*fa)(void *);     /** function pointer corresponding to thread */
       void (*f)(void);     /** function pointer corresponding to thread */
       void *stack;        /** stack pointer for this thread*/
       jmp_buf jbuf;        /** environment variable to save state of thread */
       void *returnValue;  /** value returned by thread/function if any */
       struct listNode *first;  /** list of nodes waiting for this thread to end during join call */
};


/** 
this will help in join to maintain list of threads
*/
struct listNode
{
    int threadID;
    struct listNode *next;
};

/** 
this will return the all statistics corresponding to thread
*/

struct statistics
{
    int threadID;
    double numberOfBursts;
    double totalExecTime;
    double totalSleepTime;
    double totalWaitingTime;
    int state;
    void *returnValue;
};


/**
Node of different queues
*/
struct queueNode
{
    struct thread  *thread_pointer;
    struct queueNode *nextNode;
};

/**
this will point to thread which is currently executing
*/
struct thread *currentExecutingThread;

/**
To keep track of novice queue
*/
struct queueNode *start_of_novice=NULL;
struct queueNode *end_of_novice=NULL;
/**
To keep track of ready queue
*/
struct queueNode *start_of_ready=NULL;
struct queueNode *end_of_ready=NULL;
/**
To keep track of suspended queue
*/
struct queueNode *start_of_suspended=NULL;
struct queueNode *end_of_suspended=NULL;
/**
To keep track of running queue
*/
struct queueNode *start_of_running=NULL;
struct queueNode *end_of_running=NULL;
/**
To keep track of sleeping queue
*/
struct queueNode *start_of_sleeping=NULL;
struct queueNode *end_of_sleeping=NULL;
/**
To keep track of terminated queue
*/
struct queueNode *start_of_terminated=NULL;
struct queueNode *end_of_terminated=NULL;

/**
This function will remove first thread from the given queue
*/

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


/**
This function will return 1 if thread of given id exists in the given queue else 0
*/

int findThreadinQueue(struct queueNode** start,struct queueNode** end , int thread_id)
{

    if(*start==NULL)
    {
        return 0;
    }
    else
    {
        struct queueNode *temp=*start;

        while(temp!=NULL)
        {
           if(temp->thread_pointer->threadID == thread_id)
           {
               return 1;
           }
           temp=temp->nextNode;
        }
        return 0;
    }
}

/**
This function will insert the thread at the end of the given queue
*/

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

/**
This function will return thread by removing thread of the given id
*/

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

/**
This function will return the thread from queue of given id but will not remove from the queue
*/

struct thread* getThreadFromQueueUsingThreadID(struct queueNode** start,struct queueNode** end , int thread_id)
{
    if(*start==NULL)
    {
        return NULL;
    }
    else
    {
        struct queueNode *temp=*start;

        while(temp!=NULL)
        {
           if(temp->thread_pointer->threadID == thread_id)
           {
               return temp->thread_pointer;
           }
           temp=temp->nextNode;
        }
        return NULL;
    }
}

/**
This function will create thread with arguments for a function passed in argument and will put it in novice queue
*/
int createWithArgs(void *(*fun)(void *), void *arg)
{
        struct thread *t = (struct thread * )malloc(sizeof(struct thread));
        t->threadID=n;
        n++;
        t->state=novice;
        t->numberOfBursts=0;
        t->totalExecTime=0;
        t->totalSleepTime=0;
        t->totalWaitingTime=0;
        t->funType=2;
        t->arg=arg;
        t->fa = fun;
        t->first=NULL;

        t->stack= (void *) malloc(STACK_SIZE);

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
        printf("Thread added...%d\n",t->threadID);
        return t->threadID;
}

/**
This function will create thread for a function passed as an argument and will put it in novice queue
*/

int create(void (*fun)(void))
{
        struct thread *t = (struct thread * )malloc(sizeof(struct thread));
        t->threadID=n;
        n++;
        t->state=novice;
        t->numberOfBursts=0;
        t->totalExecTime=0;
        t->totalSleepTime=0;
        t->totalWaitingTime=0;
        t->funType=1;
        t->f = fun;
        t->stack= (void *) malloc(STACK_SIZE);
        t->first=NULL;

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
/**
This function will be called very time alarm will be called and will switch btwen threads
*/
void dispatch(int a)
{
    
    //printf("Inside dispatch \n");
    try
    {
       if(kt==1)
        {
            kt=0;
            throw 20;
        }
    gettimeofday(&endExecutionTime, NULL);

    double time = (endExecutionTime.tv_sec - startExecutionTime.tv_sec)*1000000L+endExecutionTime.tv_usec - startExecutionTime.tv_usec;

    currentExecutingThread->totalExecTime=currentExecutingThread->totalExecTime+time;

    currentExecutingThread->state=ready;

    insertThreadIntoQueue(&start_of_ready,&end_of_ready,currentExecutingThread);
    if(sigsetjmp(currentExecutingThread->jbuf,1)==1)
    {
        return;
    }
    currentExecutingThread = removeThreadFromQueue(&start_of_ready,&end_of_ready);
    currentExecutingThread->numberOfBursts=currentExecutingThread->numberOfBursts+1;
    gettimeofday(&startExecutionTime, NULL);
    currentExecutingThread->state=running;

    throw 20;

    }
    catch(int e)
    {        
        if(currentExecutingThread->first != NULL)
        {
            struct listNode *temp;
            struct listNode *prev=NULL;

            temp=currentExecutingThread->first;
            while(temp!=NULL)
            {
                int exist=findThreadinQueue(&start_of_terminated,&end_of_terminated,temp->threadID);
                if(exist==1)
                {
                    if(currentExecutingThread->first==temp)
                    {
                        currentExecutingThread->first=temp->next;
                    }
                    else
                    {
                        prev->next=temp->next;
                    }
                }
                prev=temp;
                temp=temp->next;
            }
            if(currentExecutingThread->first!=NULL)
                yield(); 
        }
        else 
        {
            currentExecutingThread->numberOfBursts=currentExecutingThread->numberOfBursts+1;
            gettimeofday(&startExecutionTime, NULL);
            siglongjmp(currentExecutingThread ->jbuf,1);
        }
        
    }

}

/**
This function helps in joining thread. Calling thread will wait till the thread with threadID finishes.
*/

void JOIN(int threadID)
{
    struct listNode *temp= (struct listNode *)(malloc(sizeof(struct listNode)));
    if( threadID>n || threadID<0 || threadID == currentExecutingThread->threadID)
    {
        printf("Invalid Thread ID to join");
    }
    else
    {
        temp->threadID=threadID;
        temp->next=NULL;
        if(currentExecutingThread->first==NULL)
            currentExecutingThread->first=temp;
        else
        {

            temp->next=currentExecutingThread->first;
            currentExecutingThread->first=temp;
        }
        
        dispatch(0);
    }
}

/**
This function will return the result after executing the thread with threadID
*/

void *GetThreadResult(int threadID)
{
    int exist;
    struct thread *temp;
    exist=findThreadinQueue(&start_of_terminated,&end_of_terminated,threadID);
    if(exist==1)
    {        
        temp=getThreadFromQueueUsingThreadID(&start_of_terminated,&end_of_terminated,threadID);
        return temp->returnValue;
    }
    else
    {
        exist=0;
        while(exist!=1)
            exist=findThreadinQueue(&start_of_terminated,&end_of_terminated,threadID);

        temp=getThreadFromQueueUsingThreadID(&start_of_terminated,&end_of_terminated,threadID);
        return temp->returnValue;
    }
}

/**
This function will print statistics of thread
*/

void printStatisticsOfThread(struct thread *temp)
{
    //printf("Inside print statistics %d\n",temp->threadID);
    struct statistics *val=(struct statistics *)(malloc(sizeof(struct statistics)));

    createStats(temp,val);

    printf("Thread Id: %d\n",(val->threadID));
    printf("Number of bursts: %f\n",(val->numberOfBursts));
    printf("Total Execution Time: %f\n",(val->totalExecTime));
    printf("Total Sleep Time: %f\n",(val->totalSleepTime));
    printf("Total Waiting Time: %f\n",(val->totalWaitingTime));
    //printf("State: %s\n",val->state.c_str());
    switch(temp->state)
    {
        case 0:
            printf("State : novice\n");
            break;

        case 1:
        printf("State : ready\n");
           // stat->state="ready";
            break;

        case 2:
        printf("State : running\n");
            //stat->state="running";
            break;

        case 3:
        printf("State : sleeping\n");
            //stat->state="sleeping";
            break;

        case 4:
        printf("State : suspended\n");
           // stat->state="suspended";
            break;

        case 5:
        printf("State : terminated\n");
           // stat->state="terminated";
            break;
    }
}

/**
This function will deallocate memory allocated to temp thread
*/
void freeMemoryOfThread(struct thread *temp)
{
    
    free(temp);
}
/**
This function will put the calling thread into sleep for the specified seconds
*/
void sleep(int sec)
{
    currentExecutingThread->state=sleeping;
    currentExecutingThread->totalSleepTime=currentExecutingThread->totalSleepTime+sec;
    uint returnTime=time(0)+sec;

    while(time(0)<returnTime);
}
/**
This function will move all threads from novice queue to ready queue and will start execution
*/
void start()
{
    struct thread *t;

    struct sigaction act;
    act.sa_handler = dispatch;
    sigaction (SIGALRM, & act, 0);

    while(start_of_novice != NULL)
    {
        t=removeThreadFromQueue(&start_of_novice,&end_of_novice);
        t->state=ready;
        insertThreadIntoQueue(&start_of_ready,&end_of_ready,t);

        if(sigsetjmp(t->jbuf,1))
        {
            if(!setAlarm)
            {
                setAlarm=1;
                ualarm(SECOND,SECOND);
                //alarm(1);
            }
            tempForStack = currentExecutingThread -> stack + STACK_SIZE;
            __asm__ __volatile__("movl tempForStack,%esp");
            intermediate();
        }
    }

    currentExecutingThread = removeThreadFromQueue(&start_of_ready,&end_of_ready);
    currentExecutingThread->state=running;
    currentExecutingThread->numberOfBursts=currentExecutingThread->numberOfBursts+1;
    gettimeofday(&startExecutionTime, NULL);
    longjmp(currentExecutingThread->jbuf,1);
}

/**
This function is used to perform the work after thread finishes its execution
*/
void intermediate()
{
    void *returnValue=NULL;

    if(currentExecutingThread->funType==1)
        (*currentExecutingThread->f)();
    else if (currentExecutingThread->funType==2)
    {
        void *args=currentExecutingThread->arg;
        returnValue=(*currentExecutingThread->fa)(args);
        currentExecutingThread->returnValue=returnValue;
        printf("Value returned by function is %d \n",*(int *)(returnValue));
    }
    gettimeofday(&endExecutionTime, NULL);

    double time = (endExecutionTime.tv_sec - startExecutionTime.tv_sec)*1000000L+endExecutionTime.tv_usec - startExecutionTime.tv_usec;

    currentExecutingThread->totalExecTime=currentExecutingThread->totalExecTime+time;
    currentExecutingThread->state=terminated;

    printf("Thread %d Terminated...\n",currentExecutingThread->threadID);


    insertThreadIntoQueue(&start_of_terminated,&end_of_suspended,currentExecutingThread);

}

/**
This function will put the thread in suspend state. Thread is identified from the thread ID passed as an argument.
*/

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
        temp->state=suspended;
        insertThreadIntoQueue(&start_of_suspended,&end_of_suspended,temp);
    }
}
/**
This function will remove the thread from the suspended queue and will put in ready queue 
*/
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
        temp->state=ready;
        insertThreadIntoQueue(&start_of_ready,&end_of_ready,temp);
    }
}
/**
This function will delete the thread corresponding to given thread id, if it exists in the enviornment.
*/
void deleteThread(int threadID)
{
    struct thread *temp=NULL;
    temp=removeThreadFromQueueUsingThreadID(&start_of_novice,&end_of_novice,threadID);
    if(temp!=NULL)
    {
        printf("Thread Deleted Successfully from Novice Queue\n");
        goto free;
    }
    temp=removeThreadFromQueueUsingThreadID(&start_of_running,&end_of_running,threadID);
    if(temp!=NULL)
    {
        printf("Thread Deleted Successfully from Running Queue\n");
        goto free;
    }
    temp=removeThreadFromQueueUsingThreadID(&start_of_ready,&end_of_ready,threadID);
    if(temp!=NULL)
    {
        printf("Thread Deleted Successfully from Ready Queue\n");
        goto free;
    }
    temp=removeThreadFromQueueUsingThreadID(&start_of_suspended,&end_of_suspended,threadID);
    if(temp!=NULL)
    {
        printf("Thread Deleted Successfully from Suspended Queue\n");
        goto free;
    }
    temp=removeThreadFromQueueUsingThreadID(&start_of_sleeping,&end_of_sleeping,threadID);
    if(temp!=NULL)
    {
        printf("Thread Deleted Successfully from Sleeping Queue\n");
        goto free;
    }
     temp=getThreadFromQueueUsingThreadID(&start_of_terminated,&end_of_terminated,threadID);
    if(temp!=NULL)
    {
        printf("Thread Deleted Successfully from Terminated Queue\n");
        goto free;
    }
    printf("Could not find the required thread.\n");
    return;
    free:
        printStatisticsOfThread(temp);
        freeMemoryOfThread(temp);
}

/**
This function will return the thread ID of the calling thread.
*/

int getID()
{
    return currentExecutingThread->threadID;
}
/**
This function will remove the thread of given thtrad ID from novice queue to ready queue.
*/
void run(int threadIDToBeExecuted)
{
    struct thread *t;

    t=removeThreadFromQueueUsingThreadID(&start_of_novice,&end_of_novice,threadIDToBeExecuted);
    t->state=ready;
    insertThreadIntoQueue(&start_of_ready,&end_of_ready,t);
    if(sigsetjmp(t->jbuf,1))
    {
        tempForStack = currentExecutingThread -> stack + STACK_SIZE;
        __asm__ __volatile__("movl tempForStack,%esp");
        intermediate();
        //printf("intermediate finished and thread is... %d\n", currentExecutingThread->threadID);
        //(*currentExecutingThread->f)();
        currentExecutingThread=removeThreadFromQueue(&start_of_ready,&end_of_ready);
        //printf("set currentExecutingThread to.. %d\n", currentExecutingThread->threadID);
        kt=1;
        dispatch(0);
        //printf("set currentExecutingThread to.. %d\n", currentExecutingThread->threadID);
    }
}
/**
This function will leave its quantum and will allow others to execute.
*/
void yield()
{
    dispatch(0);
}

/**
This function will copy stats of thread in the structure which need to be returned.
*/
void createStats(struct thread *temp,struct statistics *stat)
{
    
    stat->threadID=temp->threadID;
    stat->numberOfBursts=temp->numberOfBursts;
    stat->totalExecTime=temp->totalExecTime;
    stat->totalWaitingTime=temp->totalWaitingTime;
    stat->totalSleepTime=temp->totalSleepTime;
    stat->returnValue=temp->returnValue;
    stat->state=temp->state;    
}

/**
This function will return a structure statistics will all stats corresponding to given thread ID.
*/

struct statistics* getStatus(int threadID)
{
    struct thread *temp=NULL;
    struct statistics stat;
    temp=getThreadFromQueueUsingThreadID(&start_of_novice,&end_of_novice,threadID);
    if(temp!=NULL)
    {
        createStats(temp,&stat);
        return &stat;
    }
    temp=getThreadFromQueueUsingThreadID(&start_of_running,&end_of_running,threadID);
    if(temp!=NULL)
    {
        createStats(temp,&stat);
        return &stat;
    }
    temp=getThreadFromQueueUsingThreadID(&start_of_ready,&end_of_ready,threadID);
    if(temp!=NULL)
    {
        createStats(temp,&stat);
        return &stat;
    }
    temp=getThreadFromQueueUsingThreadID(&start_of_suspended,&end_of_suspended,threadID);
    if(temp!=NULL)
    {
        createStats(temp,&stat);
        return &stat;
    }
    temp=getThreadFromQueueUsingThreadID(&start_of_sleeping,&end_of_sleeping,threadID);
    if(temp!=NULL)
    {
       createStats(temp,&stat);
        return &stat;
    }
    temp=getThreadFromQueueUsingThreadID(&start_of_terminated,&end_of_terminated,threadID);
    if(temp!=NULL)
    {
       createStats(temp,&stat);
        return &stat;
    }
    printf("Could not find the required thread.\n");
    return &stat;    
}

/**
This function will free memory and will clear all queues.
*/

void clean()
{
    struct thread *temp;
    while(start_of_novice!=NULL)
    {
        printf("Cleaning novice queue\n");
        temp=removeThreadFromQueue(&start_of_novice,&end_of_novice);
        printStatisticsOfThread(temp);
        freeMemoryOfThread(temp);
    }
    
    while(start_of_sleeping!=NULL)
    {
        printf("Cleaning sleeping queue\n");
        temp=removeThreadFromQueue(&start_of_sleeping,&end_of_sleeping);
        printStatisticsOfThread(temp);
        freeMemoryOfThread(temp);
    }
    while(start_of_suspended!=NULL)
    {
        printf("Cleaning suspended queue\n");
        temp=removeThreadFromQueue(&start_of_suspended,&end_of_suspended);
        printStatisticsOfThread(temp);
        freeMemoryOfThread(temp);
    }
    while(start_of_terminated!=NULL)
    {
        printf("Cleaning terminated queue\n");
        temp=removeThreadFromQueue(&start_of_terminated,&end_of_terminated);
        printStatisticsOfThread(temp);
        freeMemoryOfThread(temp);
    }
    while(start_of_ready!=NULL)
    {
        printf("Cleaning ready queue\n");
        temp=removeThreadFromQueue(&start_of_ready,&end_of_ready);
        printStatisticsOfThread(temp);
        freeMemoryOfThread(temp);
    }
    while(start_of_running!=NULL)
    {
        printf("Cleaning running queue\n");
        temp=removeThreadFromQueue(&start_of_running,&end_of_running);
        printStatisticsOfThread(temp);
        freeMemoryOfThread(temp);
    }
}

