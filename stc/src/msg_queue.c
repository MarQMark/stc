#include "msg_queue.h"
#include <stdlib.h>

#include <memory.h>
#include <errno.h>

MsgQueue* msg_queue_create(){
    MsgQueue* queue = (MsgQueue*)malloc(sizeof(MsgQueue));
    queue->tail = NULL;
    queue->head = NULL;
    return queue;
}

void msg_queue_enqueue(MsgQueue* msgQueue, Message* msg){
    MsgQueuePart* element = (MsgQueuePart*)malloc(sizeof(MsgQueuePart));
    element->msg = msg;
    element->next = NULL;

    if (msgQueue->head == NULL) {
        msgQueue->tail = element;
        msgQueue->head = element;
        return;
    }

    // Add the new node at the end of queue and change rear
    msgQueue->head->next = element;
    msgQueue->head = element;
}

Message* msg_queue_dequeue(MsgQueue* msgQueue){
    if(msgQueue->tail == NULL)
        return NULL;

    MsgQueuePart* element = msgQueue->tail;
    Message* msg = element->msg;

    msgQueue->tail = msgQueue->tail->next;

    if (msgQueue->tail == NULL)
        msgQueue->head = NULL;

    //free(element);
    return msg;
}

Message* msg_queue_tail(MsgQueue* msgQueue){
    if(msgQueue->tail)
        return msgQueue->tail->msg;

    return NULL;
}

uint8_t msg_queue_is_empty(MsgQueue* msgQueue){
    if(msgQueue->tail)
        return 0;

    return 1;
}

uint32_t msg_queue_size(MsgQueue* msgQueue){
    uint32_t size = 0;
    for(MsgQueuePart* element = msgQueue->tail; element != NULL; element = element->next)
        size++;
    return size;
}