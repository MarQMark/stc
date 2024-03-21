#include <cstdlib>
#include <cstring>
#include "Buffer.h"

Buffer::Buffer() {
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
