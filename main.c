#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "osqueue.h"
#include "threadPool.h"
void test_thread_pool_sanity();

int main( )
{
    printf("Hello, World!\n");

    test_thread_pool_sanity();
    return 0;
}




void hello (void* a)
{
    printf("hi\n");
}


void test_thread_pool_sanity()
{
    int i;

    ThreadPool* tp = tpCreate(5);

    for(i=0; i<10; ++i)
    {
        tpInsertTask(tp,hello,NULL);
    }

    tpDestroy(tp,1);
}

