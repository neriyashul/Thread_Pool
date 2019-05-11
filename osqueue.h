//
// Created by neriya on 5/7/19.
//

#ifndef EX4_OSQUEUE_H
#define EX4_OSQUEUE_H


typedef struct os_node
{
    struct os_node* next;
    void* data;
}OSNode;

typedef struct os_queue
{
    OSNode *head, *tail;

}OSQueue;

OSQueue* osCreateQueue();

void osDestroyQueue(OSQueue* queue);

int osIsQueueEmpty(OSQueue* queue);

void osEnqueue(OSQueue* queue, void* data);

void* osDequeue(OSQueue* queue);


#endif //EX4_OSQUEUE_H
