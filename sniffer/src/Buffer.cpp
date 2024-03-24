#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "Buffer.h"

Buffer::Buffer() {
    static_assert(sizeof(Message) < BUFFER_SIZE, "Message might not fit in single buffer -> not parsable");
    _buffers.push_back(new MBuf());
}

Buffer::~Buffer() {
    for (auto buffer : _buffers) {
        delete buffer;
    }
}

void Buffer::addData(uint8_t *data, uint32_t size) {
    MBuf* mBuf = _buffers.back();
    uint32_t freeSpace = mBuf->size - mBuf->ptr;

    if(freeSpace < size){
        printf("new buffer\n");
        memcpy(mBuf->data + mBuf->ptr, data, freeSpace);
        mBuf->ptr = mBuf->size;

        _buffers.push_back(new MBuf());
        MBuf* mBuf2 = _buffers.back();

        uint32_t remaining = size - freeSpace;
        memcpy(mBuf2->data, data + freeSpace, remaining);
        mBuf2->ptr = remaining;
    }
    else{
        memcpy(mBuf->data + mBuf->ptr, data, size);
        mBuf->ptr += size;
    }
}


int8_t Buffer::getData(uint32_t addr, uint8_t* data, uint32_t size){
    // get buffer
    uint32_t bufAddr = 0;
    MBuf* mBuf = nullptr;
    for(auto* tmp : _buffers) {
        mBuf = tmp;
        if(bufAddr + mBuf->ptr > addr){
            break;
        }

        bufAddr += mBuf->ptr;
    }

    // Address higher than size of buffer
    if(bufAddr + mBuf->ptr < addr){
        printf("Addr(0x%08X) not in Buffer(0x%08X)\n", addr, bufAddr + mBuf->ptr);
        return -1;
    }

    if(mBuf->ptr > size){
        memcpy(data, mBuf->data + (addr - bufAddr), size);
        return 1;
    }
    else if(mBuf == _buffers.back()){
        // already at last buffer (Msg not complete)
        return -1;
    }
    else{
        uint32_t part = size - mBuf->ptr;
        if(getData(addr + part, data + part, size - part)){
            memcpy(data, mBuf->data + (addr - bufAddr), part);
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

        //uint8_t* data = (uint8_t*)malloc(parsing);
        uint8_t data[parsing];
        if(getData(_last_msg_addr + i, data, parsing) == -1){
            //free(data);
            break;
        }
        printf("Buff[%d]: 0x%02X,  parsing: %d\n",_last_msg_addr + i, *data, parsing);

        if(sync < 4){
            if(*data == (uint8_t)(SER_MAGIC >> (sync * 8))){
                sync++;

                printf("sync: %d/4\n", sync);

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
                memcpy(&msg->hdr, data, sizeof(Header));
                i += sizeof(Header);

                parsing = msg->hdr.len;

                parseHdr = false;
            }
            else{
                memcpy(&msg->bdy, data, msg->hdr.len);
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

                printf("-------Message-------\n");
            }
        }

        //free(data);
    }
    delete msg;
}

