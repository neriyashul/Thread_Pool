#ifndef EX4_THREADPOOL_H
#define EX4_THREADPOOL_H

#include <sys/types.h>
#include "osqueue.h"
#include "stdio.h"


typedef struct task
{
    void (*computeFunc) (void *);
    void* param;
} Task;

typedef struct insertingTask
{
    volatile int numOfTaskThatAreInserting;
    pthread_mutex_t numOfTasMutex;
    pthread_mutex_t addTaskMutex;
    pthread_cond_t addedNewTaskCond;
    pthread_cond_t deleteThreadPool;


} InsertingTask;


typedef struct thread_pool
{
    volatile short stopAllThread;
    volatile short ignoreTheTasksInQueue;
    int numOfThreads;
    pthread_mutex_t tpMutex; // mutex to access 'taskQueue'
    OSQueue* taskQueue;
    pthread_t* threads;
    InsertingTask* insertingTask;
} ThreadPool;


ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);


#endif //EX4_THREADPOOL_H
