#ifndef STC_SERIALINTERFACE_H
#define STC_SERIALINTERFACE_H


#include <cstdio>

class SerialInterface {
public:
    SerialInterface();

    int sIFopen(const char* dev);
    void sIFclose();
    void sIFread();

    const char* getDev();

private:
    char* _dev;
    int _fd;
};


#endif //STC_SERIALINTERFACE_H
