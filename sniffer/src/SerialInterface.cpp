#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include "SerialInterface.h"

#define MAGIC 0xFEE1DEAD

SerialInterface::SerialInterface() {
    _fd = -1;
}

int SerialInterface::sIFopen(const char *dev) {
    _dev = (char*)malloc(sizeof(dev));
    strcpy(_dev, dev);

    _fd = open(_dev, NULL);

    if(_fd != -1){
        int flags = fcntl(_fd , F_GETFL, 0);
        fcntl(_fd , F_SETFL, flags | O_NONBLOCK);
    }

    return _fd;
}

void SerialInterface::sIFclose() {
    close(_fd);
}

uint8_t* SerialInterface::sIFread(uint32_t* src, uint32_t* len) {

    for(int i = 0; i < 64; i++){
        size_t bytes = read(_fd, ((uint8_t*)&_msg) + _by_recv, _by_rd);
        if(bytes == 0){
            reset();
            return nullptr;
        }
        else if(bytes == -1){
            return nullptr;
        }

        if(_sync < 4){
            uint32_t mask = 0xFFFFFFFF >> ((3 - _sync) * 8);
            if((_msg.magic & mask) == (MAGIC & mask)){
                _sync++;
                _by_recv++;

                if(_sync == 4){
                    _by_rd = sizeof(sifMsg) - 4;
                    bytes = read(_fd, ((uint8_t*)&_msg) + 4, _by_rd);
                    if(bytes != _by_rd){
                        reset();
                        return nullptr;
                    }

                    uint8_t* data = (uint8_t*)malloc(_msg.len);
                    bytes = read(_fd, data, _msg.len);
                    if(bytes == -1){
                        reset();
                        return nullptr;
                    }

                    *len = bytes;
                    *src = _msg.src;

                    reset();
                    return data;
                }
            }
            else{
                reset();
            }
        }
    }

    return nullptr;
}

const char *SerialInterface::getDev() {
    return _dev;
}

void SerialInterface::reset() {
    _sync = 0;
    _by_rd = 1;
    _by_recv = 0;
    _msg.magic = 0;
}

int SerialInterface::getFD() {
    return _fd;
}

SerialInterface::~SerialInterface() {
    sIFclose();
}
