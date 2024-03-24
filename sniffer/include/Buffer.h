#ifndef STC_BUFFER_H
#define STC_BUFFER_H

#include <vector>
#include <cstdint>
#include "message.h"

#define BUFFER_SIZE 0x4000

class Buffer {
public:
    Buffer();
    ~Buffer();

    class MBuf{
    public:
        MBuf(uint32_t size = BUFFER_SIZE) : size(size){
            data = (uint8_t*)malloc(size);
            memset(data, 0, size);
            ptr = 0;
        };
        ~MBuf(){
            free(data);
        }

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
    std::vector<Message*> _msgs;
private:

    std::vector<MBuf*> _buffers;

    uint32_t _last_msg_addr = 0;
};


#endif //STC_BUFFER_H
