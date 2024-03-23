#include "SoftwareSerial.h"

union __attribute__((__packed__)) MsgBody {
  uint32_t data[8];
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
} typedef sifMsg;


SoftwareSerial sws1(2, 0);
SoftwareSerial sws2(3, 0);

int id = 0;

void setup() {
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  Serial.begin(115200);
}


void loop() {

  Message msg;
  msg.magic = SER_MAGIC;
  msg.hdr.id = id;
  msg.hdr.len = sizeof(Body);
  memset(msg.bdy.data, 0xFF, 8);

  sifMsg msg2;
  msg2.magic = MAGIC;
  msg2.len = sizeof(Message);
  msg2.src = id % 2;

  Serial.write((uint8_t*)&msg2, msg2.len);

  id++;
  delay(1000);
}
