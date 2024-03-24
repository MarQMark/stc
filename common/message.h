#ifndef STC_MESSAGE_H
#define STC_MESSAGE_H

#include <inttypes.h>

#ifndef MESSAGE_BODY
union __attribute__((__packed__)) MsgBody {
    //TODO
    uint32_t data[8];
} typedef Body;
#endif

#define SER_MAGIC 0xDECAFBAD
#define TCF_SYN 0x1
#define TCF_ACK 0x2
#define TCF_RST 0x4

struct __attribute__((__packed__)) Header {
    uint16_t id;
    uint16_t len;
    uint32_t tcn; // Transmission Control Number
    uint8_t tcf;  // 1 : SYC; 2 : ACK
} typedef Header;

struct __attribute__((__packed__)) Message {
    uint32_t magic; // sanity check
    Header hdr;
    Body bdy;
} typedef Message;

#endif //STC_MESSAGE_H
