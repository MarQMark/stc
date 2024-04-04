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

    _size += size;
}


int8_t Buffer::getData(uint32_t addr, uint8_t* data, uint32_t size){
    if(size == 0)
        return 1;

    if(_size <= addr)
        return -1;

    // get buffer
    uint32_t bufAddr = 0;
    MBuf* mBuf = nullptr;
    for(auto* tmp : _buffers) {
        mBuf = tmp;
        if(bufAddr + mBuf->ptr >= addr){
            break;
        }

        bufAddr += mBuf->ptr;
    }

    // Address higher than size of buffer
    //if(bufAddr + mBuf->ptr <= addr){
    //    // TODO: optimize with _size, but I am too tired to tell if it actually works
    //    //printf("Addr(0x%08X) not in Buffer(0x%08X)\n", addr, bufAddr + mBuf->ptr);
    //    return -1;
    //}

    if(mBuf->ptr >= size){
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

uint8_t calc_crc(Message* msg, const uint8_t* body){
    uint8_t crc = 0;
    for (int i = 0; i < sizeof(Header) + MAGIC_LEN - 1; i++)
        crc ^= ((uint8_t*)msg)[i];

    for (int i = 0; i < msg->hdr.len; i++)
        crc ^= body[i];

    return crc;
}

void Buffer::parseMsgs(std::vector<PacketInfo*> *packets) {
    uint32_t parsing = 1;
    uint8_t sync = 0;

    bool parseHdr = true;
    Message* msg = (Message*)malloc(sizeof(Message));//new Message();
    uint32_t bufAddr = 0;

    for(uint32_t i = 0;;){

        //uint8_t* data = (uint8_t*)malloc(parsing);
        uint8_t data[parsing];
        if(getData(_last_msg_addr + i, data, parsing) == -1){
            //free(data);
            break;
        }

        if(sync < 4){
            if(*data == (uint8_t)(SER_MAGIC >> (sync * 8))){
                sync++;

                if(sync == 4){
                    parsing = sizeof(Header);
                    msg->magic = SER_MAGIC;
                    bufAddr = _last_msg_addr + i - 3;
                }
            }
            else{
                sync = 0;
            }
            i++;
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
                if(msg->hdr.len > 0){
                    msg->bdy.data = (uint8_t*) malloc(msg->hdr.len);
                    memcpy(msg->bdy.data, data, msg->hdr.len);
                    i += msg->hdr.len;
                }
                else{
                    msg->bdy.data = nullptr;
                }

                // Add new PackageInfo
                PacketInfo* packetInfo = new PacketInfo(msg, bufAddr);
                printf("%d | %d\n", msg->hdr.crc, calc_crc(msg, msg->bdy.data));
                packetInfo->validCRC = (msg->hdr.crc == calc_crc(msg, msg->bdy.data));
                packets->push_back(packetInfo);
                //printf("Msg Addr: %d(0x%X), Buffer Size: %d\n", bufAddr, bufAddr, _buffers[0]->ptr);
                _last_msg_addr += (i - 1);

                // Reset
                // Add new tmp message in case no new message is contained in buffer, since msg will always get freed
                msg = (Message*)malloc(sizeof(Message));
                memset(msg, 0, sizeof(Message));
                sync = 0;
                parsing = 1;
                parseHdr = true;
                i = 0;
            }
        }

        //free(data);
    }
    free(msg);
}

uint32_t Buffer::size() const {
    return _size;
}

