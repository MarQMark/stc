#include <cstdlib>
#include <cstring>
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

    parse_msg();
}

void Buffer::parse_msg() {
    uint32_t lastMsgPtr = _last_msg_ptr;
    uint32_t lastMsgBuf = _last_msg_buf;

    MBuf* mBuf = &_buffers[lastMsgBuf];
    uint8_t* data = mBuf->data + lastMsgPtr;

    bool eob = false; // end of buffer
    uint8_t sync = 0;

    bool parseHdr = true;
    auto* msg = new Message();

    for(uint32_t i = 0; !eob; i++){

        if(lastMsgPtr + i >= mBuf->ptr){
            // no message found
            eob = true;
            continue;
        }
        else if(lastMsgPtr + i >= mBuf->size){
            lastMsgBuf++;

            mBuf = &_buffers[lastMsgBuf];
            data = mBuf->data;
            lastMsgPtr = 0;
            i = 0;

            // No part of Message
            if(sync == 0){
                _last_msg_ptr = lastMsgPtr;
                _last_msg_buf = lastMsgBuf;
            }
        }

        if(sync < 4){
            if(*data == (uint8_t)(SER_MAGIC >> ((3 - sync) * 8))){
                sync++;
            }
            else{
                sync = 0;
            }
            data++;
        }
        else {

            uint32_t bufRem = mBuf->ptr - (lastMsgPtr + i);
            if(parseHdr){

                // check if message split into 2 buffers
                if(sizeof(Header) > bufRem){
                    uint32_t msgRem = sizeof(Header) - bufRem;
                    if(_last_msg_buf + 1 < _buffers.size() && _buffers[lastMsgBuf].ptr > msgRem){
                        // copy rest of old buffer
                        memcpy(msg, data - 4, bufRem + 4);
                        // copy port in new buffer
                        lastMsgBuf++;
                        mBuf = &_buffers[lastMsgBuf];
                        data = mBuf->data;
                        memcpy(msg, data, msgRem);

                        // reset ptr
                        i = 0;
                        lastMsgPtr = msgRem;
                    }
                    else{
                        //Header not completely in buffer
                        eob = true;
                        continue;
                    }
                }
                else{
                    // copy Header and  Magic Number (4)
                    memcpy(msg, data - 4, 4 + sizeof(Header));
                    lastMsgPtr += sizeof(Header) + 4;
                }

                parseHdr = false;
            }
            else{
                if(msg->hdr.len > bufRem){
                    uint32_t msgRem = msg->hdr.len - bufRem;
                    if(_last_msg_buf + 1 < _buffers.size() && _buffers[lastMsgBuf].ptr > msgRem){
                        // copy rest of old buffer
                        memcpy(msg, data - 4, bufRem + 4);
                        // copy port in new buffer
                        lastMsgBuf++;
                        mBuf = &_buffers[lastMsgBuf];
                        data = mBuf->data;
                        memcpy(msg, data, msgRem);

                        // reset ptr
                        i = 0;
                        lastMsgPtr = msgRem;
                    }
                    else{
                        // Body not completely in buffer
                        eob = true;
                        continue;
                    }
                }
                else{
                    // copy Body
                    memcpy(msg, data, msg->hdr.len);
                    lastMsgPtr += msg->hdr.len;
                }

                // Add message
                _msgs.push_back(msg);
                _last_msg_buf = lastMsgBuf;
                _last_msg_ptr = lastMsgPtr;
                // Add new tmp message in case no new message is contained in buffer, since msg will always get freed
                msg = new Message();
                sync = 0;
                parseHdr = true;
            }
        }
    }

    delete msg;
}
