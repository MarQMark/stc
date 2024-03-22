#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include "SerialInterface.h"

SerialInterface::SerialInterface() {

}

int SerialInterface::sIFopen(const char *dev) {
    _dev = (char*)malloc(sizeof(dev));
    strcpy(_dev, dev);

    _fd = open(_dev, NULL);
    return _fd;
}

void SerialInterface::sIFclose() {
    close(_fd);
}

void SerialInterface::sIFread() {

}

const char *SerialInterface::getDev() {
    return _dev;
}
