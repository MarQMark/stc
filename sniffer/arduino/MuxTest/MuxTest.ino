#include "SoftwareSerial.h"

union __attribute__((__packed__)) MsgBody {
  char data[8];
} typedef Body;

#define SER_MAGIC 0xDECAFBAD
#define TCF_SYN 0x1
#define TCF_ACK 0x2
#define TCF_RST 0x4

struct __attribute__((__packed__)) Header {
    uint16_t id;
    uint16_t len;
    uint32_t tcn; // Transmission Control Number
    uint8_t tcf;  // 1 : SYC; 2 : ACK
    uint8_t crc;
} typedef Header;

struct __attribute__((__packed__)) Message {
    uint32_t magic; // sanity check
    Header hdr;
    Body bdy;
} typedef Message;


struct __attribute__((__packed__)) sifMsg{
#define MAGIC 0xFEE1DEAD
  uint32_t magic;
  uint32_t len;
  uint32_t src;
  Message msg;
} typedef sifMsg;

/*uint8_t calc_crc(Message* msg){
    uint8_t crc = 0;
    for(int i = 0; i < sizeof(Header) + 4 - 1; i++)
        crc ^= ((uint8_t*)msg)[i];

    for(int i = 0; i < msg->hdr.len; i++)
        crc ^= ((uint8_t*)msg)[i + sizeof(Header) + 4];

    return crc;
}*/

// Why?
uint8_t calc_crc(Message msg){
    uint8_t crc = 0;
    for(int i = 0; i < sizeof(Header) + 4 - 1; i++)
        crc ^= ((uint8_t*)&msg)[i];

    for(int i = 0; i < msg.hdr.len; i++)
        crc ^= ((uint8_t*)&msg)[i + sizeof(Header) + 4];

    return crc;
}

int id;

void setup() {
  id = 0;
  Serial.begin(115200);
}


void loop() {

  sifMsg msg;
  msg.magic = MAGIC;
  msg.len = sizeof(Message);
  msg.src = id % 2;

  memset(&msg.msg, 0, sizeof(Message));
  msg.msg.magic = SER_MAGIC;
  msg.msg.hdr.id = 1;//id;
  msg.msg.hdr.len = sizeof(Body);
  strcpy(msg.msg.bdy.data, "TestData");

  uint8_t crc = calc_crc(msg.msg);
  msg.msg.hdr.crc = crc;

  Serial.write((uint8_t*)&msg, msg.len + 12);

  id++;
  delay(1000);
}
