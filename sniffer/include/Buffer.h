#ifndef STC_BUFFER_H
#define STC_BUFFER_H

#include <vector>
#include <cstdint>
#include "message.h"

#define BUFFER_SIZE 0x400

class Buffer {
public:
    Buffer();
    ~Buffer();

    class MBuf{
    public:
        MBuf(uint32_t size = BUFFER_SIZE) : size(size){
            data = (uint8_t*)malloc(size);
            ptr = 0;
        };

        uint8_t* data;
        uint32_t size;
        uint32_t ptr;
    };

    void addData(uint8_t* data, uint32_t size);


private:
    void parse_msg();

    std::vector<MBuf> _buffers;

    std::vector<Message*> _msgs;
    uint32_t _last_msg_buf = 0;
    uint32_t _last_msg_ptr = 0;
};


#endif //STC_BUFFER_H
