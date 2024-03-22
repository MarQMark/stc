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

    /* int8_t getData(uint8_t* data, uint32_t size)
     *
     * returns:
     *   1: success
     *  -1: failure
     */
    int8_t getData(uint32_t addr, uint8_t* data, uint32_t size);

    void parseMsgs();
private:

    std::vector<MBuf> _buffers;

    std::vector<Message*> _msgs;
    uint32_t _last_msg_addr = 0;
};


#endif //STC_BUFFER_H
