#include "serial.h"
#include "stdlib.h"

int serial_init(Serial* serial){
    serial_send_rst(serial);
    serial_reset_hard(serial);

#if _LINUX
    // open serial fd
    serial->fd = open("/dev/serial0", O_RDWR);
    if(serial->fd  < 0){
        return -1;
    }

    int flags = fcntl(serial->fd , F_GETFL, 0);
    fcntl(serial->fd , F_SETFL, flags | O_NONBLOCK);

    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    context.last_time = spec.tv_nsec;
#elif _ARDUINO
    context.last_time = millis();
#endif

    return 0;
}

uint8_t serial_calc_crc(Message* msg){
    uint8_t crc = 0;
    for(uint32_t i = 0; i < sizeof(Header) + MAGIC_LEN - 1; i++)
        crc ^= ((uint8_t*)msg)[i];

    for(uint32_t i = 0; i < msg->hdr.len; i++)
        crc ^= ((uint8_t*)msg)[i + sizeof(Header) + MAGIC_LEN];

    return crc;
}

void serial_clean(Serial* serial){

    while(!msg_queue_is_empty(serial->tx_queue)){
        Message *msg = msg_queue_dequeue(serial->tx_queue);
        free(msg);
    }

#if _LINUX
    close(serial.fd);
#endif
}

void serial_reset(Serial* serial) {
    serial->sync = 0;
    serial->bytes_to_read = 1;
    serial->bytes_read = 0;
    serial->read_hdr = false;
    serial->msg.magic = 0;
    serial->resend_timeout = 0;
    serial->read_timeout = SER_RD_TIMEOUT;
}

void serial_reset_hard(Serial* serial){
#if _LINUX
    tcflush(serial->fd, TCOFLUSH);
#elif _ARDUINO
    Serial.flush();
#endif

    serial_reset(serial);
    serial->tcn_nxt = 0;
    serial->rx_tcn = 0;
    serial->tx_tcn = 0;
    serial->tx_tcn_cnt = 0;

    // Set new TCN in already queued msg
    for(MsgQueuePart* element = serial->tx_queue->tail; element != NULL; element = element->next){
        element->msg->hdr.tcn = serial->tx_tcn_cnt;
        serial->tx_tcn_cnt++;
    }
}


void read_sync(Serial* serial){
    uint32_t mask = 0xFFFFFFFF >> ((3 - serial->sync) * 8);
    if((serial->msg.magic & mask) == (SER_MAGIC & mask)){
        serial->sync++;
        serial->bytes_read++;

        if(serial->sync == 4){
            serial->bytes_to_read = sizeof(Header);
        }
    }
    else{
        serial_reset(serial);
    }
}

void read_hdr(Serial* serial){
    serial->read_hdr = true;
    serial->bytes_to_read = serial->msg.hdr.len;

    if(serial->msg.hdr.len > sizeof(Body))
        serial_reset(serial);
}

void read_bdy(Serial* serial){
    serial->read_bdy = true;

    if(serial->msg.hdr.crc != serial_calc_crc(&serial->msg)){
        // Throw message into garbage (invalid CRC)
        serial_msg_pro(serial);
    }
    else if(serial->msg.hdr.tcf & TCF_RST){
        serial_reset_hard(serial);
    }
    else if(serial->msg.hdr.tcf & TCF_SYN){
        // SYN
        if(serial->tcn_nxt == serial->msg.hdr.tcn){
            serial->rx_tcn = serial->msg.hdr.tcn;
            serial_send_ack(serial);
            serial->tcn_nxt++;
        }
        else{
            // flush buffer if received older tcn
#if _LINUX
            tcflush(serial->fd, TCOFLUSH);
#elif _ARDUINO
            Serial.flush();
#endif
            serial_send_ack(serial);
            // Throw message into garbage
            serial_msg_pro(serial);
        }
    }
    else if(serial->msg.hdr.tcf & TCF_ACK){
        //ACK
        if(serial->msg.hdr.tcn == serial->tx_tcn){
            serial->tx_tcn++;
            serial->resend_timeout = 0;
            serial_msg_pro(serial);
        }
    }

    serial_reset(serial);
}

void serial_rx(Serial* serial) {

    if(serial->bytes_to_read != 0){
#if _LINUX
        size_t bytes = read(serial->fd, ((uint8_t*)&serial->msg) + serial->bytes_read, serial->bytes_to_read);
#elif _ARDUINO
        size_t bytes = Serial.readBytes(((uint8_t*)&serial->msg) + serial->bytes_read, serial->bytes_to_read);
#else
        size_t bytes = 0;
#endif

        // Sync
        if(serial->sync < 4 && bytes > 0){
            read_sync(serial);
        }
        else if(bytes > 0){
            serial->bytes_to_read -= bytes;
            serial->bytes_read += bytes;
        }
    }
    else if(!serial->read_hdr){
        // Header was read
        read_hdr(serial);
    }
    else{
        // Body was read
        read_bdy(serial);
    }


    // Read Timout stuff
    if(serial->read_hdr && !serial->read_bdy){
        serial->read_timeout -= (int64_t)serial->dt_millis;

        if(serial->read_timeout <= 0){
            serial_reset(serial);
            serial->read_timeout = SER_RD_TIMEOUT;
        }
    }
    else if(serial->read_hdr && serial->read_bdy){
        serial->read_bdy = false;
        serial->read_timeout = SER_RD_TIMEOUT;
    }

    // Prevent Overflow
    if(serial->tx_tcn == 0xFFFFFFFF || serial->rx_tcn == 0xFFFFFFFF){
        serial_send_rst(serial);
        serial_reset_hard(serial);
    }
}

void serial_tx(Serial* serial){

    if(msg_queue_is_empty(serial->tx_queue))
        return;

    Message* msg = msg_queue_tail(serial->tx_queue);
    if(msg->hdr.tcn < serial->tx_tcn){
        msg = msg_queue_dequeue(serial->tx_queue);
        //free(msg);
        return;
    }

    if(serial->resend_timeout <= 0){
#if _LINUX
        write(serial->fd, (uint8_t*)&msg, 4 + sizeof(Header) + msg.hdr.len);
#elif _ARDUINO
        Serial.write((uint8_t*)&msg, 4 + sizeof(Header) + msg.hdr.len);
#endif
        serial->resend_timeout = SER_RSD_TIMEOUT;
    }
    else{
        serial->resend_timeout -= (int64_t)serial->dt_millis;
    }
}

void serial_update(Serial* serial){
#if _LINUX
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    if(spec.tv_nsec - context->last_time < 1000000)
        context->dt_millis = 1;
    else
        context->dt_millis = (spec.tv_nsec - context->last_time) / 1000000;
    context->last_time = spec.tv_nsec;
#elif _ARDUINO
    context.dt_millis = millis() - context.last_time;
    context.last_time = millis();
#endif

    serial_rx(serial);
    serial_tx(serial);
}

void serial_msg_pro(Serial* serial){
    serial->read_bdy = false;
    serial_reset(serial);
}

void serial_send(Serial* serial, Message* msg){
    msg->hdr.tcn = serial->tx_tcn_cnt;
    serial->tx_tcn_cnt++;
    msg->hdr.crc = serial_calc_crc(msg);
    msg_queue_enqueue(serial->tx_queue, msg);
}

void serial_send_ack(Serial* serial){
    Message msg;
    msg.magic = SER_MAGIC;
    msg.hdr.tcn = serial->rx_tcn;
    msg.hdr.tcf = TCF_ACK;
    msg.hdr.id = 0;
    msg.hdr.len = 0;
    msg.hdr.crc = serial_calc_crc(&msg);

#if _LINUX
    write(serial->fd, (uint8_t*)&msg, 4 + sizeof(Header) + msg.hdr.len);
#elif _ARDUINO
    Serial.write((uint8_t*)&msg, 4 + sizeof(Header) + msg.hdr.len);
#endif
}

void serial_send_rst(Serial* serial){
    Message msg;
    msg.magic = SER_MAGIC;
    msg.hdr.tcn = 0;
    msg.hdr.tcf = TCF_SYN | TCF_RST;
    msg.hdr.len = 0;
    msg.hdr.id = 0;
    msg.hdr.crc = serial_calc_crc(&msg);

#if _LINUX
    write(serial->fd, (uint8_t*)&msg, 4 + sizeof(Header) + msg.hdr.len);
#elif _ARDUINO
    Serial.write((uint8_t*)&msg, 4 + sizeof(Header) + msg.hdr.len);
#endif
}
