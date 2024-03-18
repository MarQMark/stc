#include "msg_queue.h"
#include <stdlib.h>

void msg_queue_enqueue(MsgQueue* msgQueue, Message* msg){
    MsgQueuePart* element = (MsgQueuePart*)malloc(sizeof(MsgQueuePart));
    element->msg = msg;
    element->head = NULL;

    if(msgQueue->head){
        element->tail = msgQueue->head;
    }
    else{
        element->tail = NULL;
        msgQueue->tail = element;
    }

    msgQueue->head = element;
}

Message* msg_queue_dequeue(MsgQueue* msgQueue){
    if(msgQueue->tail){
        MsgQueuePart* element = msgQueue->tail;
        Message* msg = element->msg;

        if(msgQueue->tail->head){
            // At least two elements in queue
            msgQueue->tail = msgQueue->tail->head;
        }
        else{
            // Only one element in queue
            msgQueue->tail = NULL;
            msgQueue->head = NULL;
        }
        free(element);
        return msg;
    }
    else{
        // Queue empty
        return NULL;
    }
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