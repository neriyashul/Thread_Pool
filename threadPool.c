// Neriya Shulman 208275024

#include <pthread.h>
#include <unistd.h>
#include "threadPool.h"
#include "stdlib.h"

/**
 * The function release unlock mutex and call with return value.
 * @param m - pthread_mutex_t*.
 * @retval - void*.
 */
void releaseMutexAndExitThread(pthread_mutex_t* m, void* retVal) {
    pthread_mutex_unlock(m);
    pthread_exit(retVal);
}


/**
 * The function:
 * 1) Waiting for a new mission to enter the queue.
 * 2) Run until stopAllThread has changes and the queue is empty or ignoring it.
 *
 * @param arg_threadPool - ThreadPool*.
 * @return NULL (void*).
 */
void* runThread(void* arg_threadPool) {
    ThreadPool* tp = (ThreadPool*) arg_threadPool;
    OSQueue* taskQueue = tp->taskQueue;

    while(!tp->stopAllThread || !osIsQueueEmpty(tp->taskQueue)) {

        pthread_mutex_lock(&tp->tpMutex);
        while (osIsQueueEmpty(taskQueue)) {
            // if ended
            if (tp->stopAllThread) {
                releaseMutexAndExitThread(&tp->tpMutex, 0);
            }

            /* block this thread until another thread signals that new task inserted. */
            pthread_cond_wait(&tp->insertingTask->addedNewTaskCond, &tp->tpMutex);
        }

        if (tp->ignoreTheTasksInQueue && tp->stopAllThread) {
            releaseMutexAndExitThread(&tp->tpMutex, 0);
        }

        Task *task = (Task *) osDequeue(taskQueue);
        pthread_mutex_unlock(&tp->tpMutex);
        task->computeFunc(task->param);

        free(task);
    }

    return NULL;
}

/**
 * The function create new InsertingTask*.
 * @return InsertingTask*.
 */
InsertingTask* initInsertingTask() {
    InsertingTask* it = (InsertingTask*) malloc(sizeof(InsertingTask));
    if (it == NULL) {
        return NULL;
    }
    it->numOfTaskThatAreInserting = 0;
    pthread_cond_init(&it->addedNewTaskCond, NULL);
    pthread_cond_init(&it->deleteThreadPool, NULL);

    pthread_mutex_init(&it->addTaskMutex, NULL);
    pthread_mutex_init(&it->numOfTasMutex, NULL);

    return it;
}


/**
 * The function create new ThreadPool*.
 * @param numOfThreads - int.
 * @return ThreadPool*.
 */
ThreadPool* tpCreate(int numOfThreads){
    ThreadPool* tp = (ThreadPool*) malloc(sizeof(ThreadPool));
    if (tp == NULL) {
        write(STDERR_FILENO, "‪Error in system call‬‬", 20);
        return NULL;
    }
    tp->taskQueue = osCreateQueue();

    pthread_mutex_init(&tp->tpMutex, NULL);

    tp->numOfThreads = numOfThreads;
    tp->stopAllThread = 0;
    tp->ignoreTheTasksInQueue = 0;

    tp->threads = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads);
    tp->insertingTask = initInsertingTask();

    if (tp->threads == NULL || tp->insertingTask == NULL) {
        write(STDERR_FILENO, "Error in system call‬‬", 20);
        free(tp);
        return NULL;
    }

    int i;
    for (i = 0; i < numOfThreads; ++i) {
        pthread_create(&tp->threads[i], NULL, runThread, tp);
    }

    return tp;
}


/**
 * The function destroys the mutexes and release allocated memory.
 * @param threadPool - ThreadPool*.
 */
void releaseThreadPoolObjects(ThreadPool* threadPool) {
    // destroy mutexes.
    pthread_mutex_destroy(&threadPool->insertingTask->numOfTasMutex);
    pthread_mutex_destroy(&threadPool->insertingTask->addTaskMutex);
    pthread_mutex_destroy(&threadPool->tpMutex);

    // destroy queue.
    osDestroyQueue(threadPool->taskQueue);

    // free the rest.
    free(threadPool->threads);
    free(threadPool->insertingTask);
    free(threadPool);
}


/**
 * The function close the threads and destroys the given threadPool.
 * @param threadPool - ThreadPool.
 * @param shouldWaitForTasks  - int:
 *        determines whether or not to wait for the tasks in the queue before closing the threads.
 */
void destroyThreadPool(ThreadPool* threadPool, int shouldWaitForTasks) {
    threadPool->stopAllThread = 1;

    if (!shouldWaitForTasks) {
        threadPool->ignoreTheTasksInQueue = 1;
    }


    pthread_mutex_lock(&threadPool->insertingTask->numOfTasMutex);



    //if there is task that is already inserting now to the queue:
    if (threadPool->insertingTask->numOfTaskThatAreInserting > 0) {
        // wait until the inserting end (before destroy the threadPool)
        pthread_cond_wait(&threadPool->insertingTask->deleteThreadPool,
                &threadPool->insertingTask->numOfTasMutex);
    }

    int i;
    for (i = 0; i < threadPool->numOfThreads; ++i) {
        pthread_join(threadPool->threads[i], NULL);
    }

    releaseThreadPoolObjects(threadPool);
}


/**
 * The function close the threads and destroys the given threadPool.
 * NOTE: The function checks at the time of the destruction that a call to 'tpDestroy' does nothing.
 *
 * @param threadPool - ThreadPool.
 * @param shouldWaitForTasks  - int:
 *        determines whether or not to wait for the tasks in the queue before closing the threads.
 */
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
    static int  isAlreadyDestroyed = 0;

    if (isAlreadyDestroyed == 0) {
        ++isAlreadyDestroyed;
        destroyThreadPool(threadPool, shouldWaitForTasks);
        isAlreadyDestroyed = 0;
    }
}


/**
 * The function creates new Task, which calls to the given function with the given parameter.
 * The function inserts the Task to the taskQueue in 'threadPool',
 *          a parameter it received, and signals that new task added.
 * @param threadPool -ThreadPool*.
 * @param computeFunc - void (*computeFunc) (void *). (pointer to function).
 * @param param - void*.
 * @return int. (0 = success, -1 = failure).
 */
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {
    if (threadPool == NULL) {
        return -1;
    }

    InsertingTask* it = threadPool->insertingTask;

    pthread_mutex_lock(&it->numOfTasMutex);
    ++it->numOfTaskThatAreInserting;
    pthread_mutex_unlock(&it->numOfTasMutex);

    Task* task = (Task*) malloc(sizeof(Task));
    if (task == NULL) {
        write(STDERR_FILENO, "Error in system call‬‬", 20);
        return -1;
    }
    task->computeFunc = computeFunc;
    task->param = param;

    //TODO: check memory leak.

    if (threadPool->stopAllThread == 0) {

        pthread_mutex_lock(&threadPool->tpMutex);
        osEnqueue(threadPool->taskQueue, task);
        pthread_cond_signal(&it->addedNewTaskCond);
        pthread_mutex_unlock(&threadPool->tpMutex);

        pthread_mutex_lock(&it->numOfTasMutex);
        --it->numOfTaskThatAreInserting;
        pthread_mutex_unlock(&it->numOfTasMutex);

        if (it->numOfTaskThatAreInserting == 0) {
            pthread_cond_signal(&it->deleteThreadPool);
        }

        return 0;
    }

    // if there was a problem - free the memory and return -1.
    free(task);
    return -1;
}

