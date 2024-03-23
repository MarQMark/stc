#ifndef STC_SERIALINTERFACE_H
#define STC_SERIALINTERFACE_H


#include <cstdio>
#include <cstdint>

class SerialInterface {
public:
    SerialInterface();
    ~SerialInterface();

    int sIFopen(const char* dev);
    void sIFclose();
    uint8_t* sIFread(uint32_t* src, uint32_t* len);

    const char* getDev();
    int getFD();

private:
    void reset();

    struct __attribute__((__packed__)) sifMsg{
        uint32_t magic;
        uint32_t len;
        uint32_t src;
    } typedef sifMsg;

    char* _dev;
    int _fd;

    uint8_t _sync = 0;
    uint32_t _by_rd = 1;
    uint32_t _by_recv = 0;
    sifMsg _msg;
};


#endif //STC_SERIALINTERFACE_H
