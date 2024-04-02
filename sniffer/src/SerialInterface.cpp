#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <termios.h>
#include "SerialInterface.h"

#define MAGIC 0xFEE1DEAD

SerialInterface::SerialInterface() {
    _fd = -1;
}

int
set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("error %d from tcgetattr\n", errno);
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        printf("error %d from tcsetattr\n", errno);
        return -1;
    }
    return 0;
}

void
set_blocking (int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("error %d from tggetattr\n", errno);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        printf("error %d setting term attributes\n", errno);
}

int SerialInterface::sIFopen(const char *dev) {
    _dev = (char*)malloc(sizeof(dev));
    strcpy(_dev, dev);

    _fd = open(_dev, NULL);

    if(_fd != -1){
        int flags = fcntl(_fd , F_GETFL, 0);
        fcntl(_fd , F_SETFL, flags | O_NONBLOCK);

        set_interface_attribs (_fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
        set_blocking (_fd, 0);                // set no blocking
    }

    return _fd;
}

void SerialInterface::sIFclose() {
    close(_fd);
}

uint8_t* SerialInterface::sIFread(uint32_t* src, uint32_t* len) {

    for(int i = 0; i < 64; i++){
        int bytes = read(_fd, ((uint8_t*)&_msg) + _by_recv, _by_rd);
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

uint8_t *SerialInterface::sIFread(uint32_t *len) {
    uint8_t* data = (uint8_t*)malloc(256);

    int bytes = read(_fd, data, 256);
    if(bytes <= 0){
        return nullptr;
    }
    *len = bytes;

    return data;
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


