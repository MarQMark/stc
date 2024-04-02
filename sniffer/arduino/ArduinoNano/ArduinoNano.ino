#include "SoftwareSerial.h"

#define RX_PIN 2
#define BUF_SIZE 256

struct __attribute__((__packed__)) _Msg{
#define MAGIC 0xFEE1DEAD
  uint32_t magic;
  uint32_t len;
  uint32_t src;
  uint8_t data[BUF_SIZE];
} typedef Msg;


SoftwareSerial sws(RX_PIN, 10);
Msg msg;

void setup() {
  pinMode(RX_PIN, INPUT);

  Serial.begin(115200);

  sws.begin(115200);
  sws.setTimeout(1);
}

void loop() {

  while(sws.available() > 0 ){
    int recv = sws.readBytes(msg.data, BUF_SIZE);
    if(recv > 0){
      Serial.println(recv);
      msg.magic = MAGIC;
      msg.src = 2;
      msg.len = recv;
      Serial.write((uint8_t*)&msg, msg.len + 12);
      delay(100);
    }
  }
  
  delay(100);
}
