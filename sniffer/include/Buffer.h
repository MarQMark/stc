#ifndef STC_BUFFER_H
#define STC_BUFFER_H

#include <vector>
#include <cstdint>


class Buffer {
public:
    Buffer();
    ~Buffer();

    class MBuf{
    public:
        MBuf(uint32_t size = 0x400) : size(size){
            data = (uint8_t*)malloc(size);
            ptr = 0;
        };

        uint8_t* data;
        uint32_t size;
        uint32_t ptr;
    };

    void addData(uint8_t* data, uint32_t size);


private:

    std::vector<MBuf> _buffers;
    std::vector<void*> _msgs;
};


#endif //STC_BUFFER_H
