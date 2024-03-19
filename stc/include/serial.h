#ifndef STC_SERIAL_H
#define STC_SERIAL_H

#include <stdbool.h>
#include <inttypes.h>
#include "msg_queue.h"


#define SER_RD_TIMEOUT 1000
#define SER_RSD_TIMEOUT 1000

#if _LINUX
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>
#elif _ARDUINO
#include "Arduino.h"
#endif

struct _Serial {
    Message msg;
    uint32_t bytes_to_read;
    uint32_t bytes_read;

    bool read_hdr;
    bool read_bdy;
    int64_t read_timeout;

    uint8_t sync; // each byte received from magic number increases sync by one until it is four

    uint32_t tx_tcn;
    uint32_t tx_tcn_cnt;
    uint32_t rx_tcn;

    uint32_t tcn_nxt; // 1 = TCN already received (duplicate message); 0 = new message

    MsgQueue tx_queue;
    int64_t resend_timeout;

    int64_t last_time;
    uint64_t dt_millis;

#ifdef _LINUX
    int32_t fd;
#endif
}  typedef Serial;

int serial_init(Serial* context);
void serial_reset(Serial* context);
void serial_reset_hard(Serial* context);
void serial_update(Serial* context);
void serial_send(Serial* context, Message* msg);
void serial_send_rst(Serial* context);
void serial_send_ack(Serial* context);
void serial_msg_pro(Serial* context);


#endif //STC_SERIAL_H
