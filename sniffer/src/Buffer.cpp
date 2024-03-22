#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "Buffer.h"

Buffer::Buffer() {
    static_assert(sizeof(Message) < BUFFER_SIZE, "Message might not fit in single buffer -> not parsable");
    _buffers.emplace_back();
}

Buffer::~Buffer() {
    for (auto buffer : _buffers) {
        free(buffer.data);
    }
}

void Buffer::addData(uint8_t *data, uint32_t size) {
    MBuf* mBuf = &_buffers.back();
    uint32_t freeSpace = mBuf->size - mBuf->ptr;

    if(freeSpace < size){
        memcpy(mBuf->data + mBuf->ptr, data, freeSpace);
        mBuf->ptr = mBuf->size;

        _buffers.emplace_back();
        MBuf* mBuf2 = &_buffers.back();

        uint32_t remaining = size - freeSpace;
        memcpy(mBuf2->data, data + freeSpace, remaining);
        mBuf2->ptr = remaining;
    }
    else{
        memcpy(mBuf->data + mBuf->ptr, data, size);
        mBuf->ptr += mBuf->size;
    }
}


int8_t Buffer::getData(uint32_t addr, uint8_t* data, uint32_t size){
    // get buffer
    uint32_t bufAddr = 0;
    MBuf* mBuf = nullptr;
    for (int i = 0; i < _buffers.size(); ++i) {
        mBuf = &_buffers[i];
        if(bufAddr + mBuf->ptr < addr){
            break;
        }

        bufAddr += mBuf->ptr;
    }

    // Address higher than size of buffer
    if(bufAddr + mBuf->ptr > addr){
        printf("Addr(0x%08X) not in Buffer(0x%08X)\n", addr, bufAddr + mBuf->ptr);
        return -1;
    }

    if(mBuf->ptr > size){
        memcpy(data, mBuf + (addr - bufAddr), size);
        return 1;
    }
    else{
        uint32_t part = size - mBuf->ptr;
        if(getData(addr + part, data + part, size - part)){
            memcpy(data, mBuf + (addr - bufAddr), part);
        }
        else{
            return -1;
        }
    }

    return 1;
}

void Buffer::parseMsgs() {
    uint32_t parsing = 1;
    uint8_t sync = 0;

    bool parseHdr = true;
    auto* msg = new Message();

    for(uint32_t i = 0;; i++){

        uint8_t* data = (uint8_t*)malloc(parsing);
        if(getData(_last_msg_addr + i, data, parsing) == -1){
            free(data);
            break;
        }


        if(sync < 4){
            if(*data == (uint8_t)(SER_MAGIC >> ((3 - sync) * 8))){
                sync++;

                if(sync == 4){
                    parsing = sizeof(Header);
                    msg->magic = SER_MAGIC;
                }
            }
            else{
                sync = 0;
            }
        }
        else {

            if(parseHdr){
                // copy Header
                memcpy(msg, data, sizeof(Header));
                i += sizeof(Header);

                parsing = msg->hdr.len;

                parseHdr = false;
            }
            else{
                memcpy(msg, data, msg->hdr.len);
                i += msg->hdr.len;

                // Add message
                _msgs.push_back(msg);
                _last_msg_addr += i;

                // Reset
                // Add new tmp message in case no new message is contained in buffer, since msg will always get freed
                msg = new Message();
                sync = 0;
                parsing = 1;
                parseHdr = true;
            }
        }

        free(data);
    }
    delete msg;
}

