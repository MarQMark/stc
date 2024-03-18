#ifndef STC_MSG_QUEUE_H
#define STC_MSG_QUEUE_H

#include "message.h"

struct MsgQueuePart{
    struct MsgQueuePart* tail;
    struct MsgQueuePart* head;
    Message* msg;
} typedef MsgQueuePart;

struct MsgQueue{
    MsgQueuePart* tail;
    MsgQueuePart* head;
} typedef MsgQueue;


Message* msg_get_test();
Message* msg_get_rst();
Message* msg_get_home();
Message* msg_get_calibrate();
Message* msg_get_field(uint8_t field, uint32_t speed, uint8_t magnet);
Message* msg_get_mov(uint8_t dir, uint32_t steps, uint32_t speed, uint8_t magnet);
Message* msg_get_btn(uint8_t id, uint32_t state);

void msg_queue_enqueue(MsgQueue* msgQueue, Message* msg);
Message* msg_queue_dequeue(MsgQueue* msgQueue);
Message* msg_queue_tail(MsgQueue* msgQueue);
uint8_t msg_queue_is_empty(MsgQueue* msgQueue);
uint32_t msg_queue_size(MsgQueue* msgQueue);

#endif //STC_MSG_QUEUE_H
