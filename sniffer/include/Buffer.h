#ifndef STC_BUFFER_H
#define STC_BUFFER_H

#include <vector>
#include <cstdint>
#include <string>

#define MESSAGE_BODY
union __attribute__((__packed__)) MsgBody {
    uint8_t* data = nullptr;
} typedef Body;

#include "message.h"

#define BUFFER_SIZE 0x4000


struct PacketInfo{
    PacketInfo() = default;
    PacketInfo(Message* msg, uint32_t bufAddr){
        this->bufAddr = bufAddr;
        this->msg = msg;
    };

    uint32_t bufAddr;
    std::string src;
    uint64_t timestamp;
    Message* msg;
    bool validCRC;
};


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

    void parseMsgs(std::vector<PacketInfo*> *);

    uint32_t size() const;
private:

    std::vector<MBuf*> _buffers;

    uint32_t _last_msg_addr = 0;
    uint32_t _size = 0;
};


#endif //STC_BUFFER_H
