#include "SoftwareSerial.h"

struct __attribute__((__packed__)) sifMsg{
#define MAGIC 0xFEE1DEAD
  uint32_t magic;
  uint32_t len;
  uint32_t src;
  uint8_t data[256];
} typedef sifMsg;


SoftwareSerial sws1(2, 10);
SoftwareSerial sws2(3, 11);


void setup() {
  pinMode(2, INPUT);
  pinMode(3, INPUT);


  Serial.begin(115200);
//Serial.println("Init");

  sws1.begin(115200);
  sws2.begin(115200);
  sws1.setTimeout(1);
  sws2.setTimeout(1);
}

void loop() {

  //sws1.listen();
  sifMsg msg1;
  while(sws1.available() > 0 ){
    int recv1 = sws1.readBytes(msg1.data, 256);
    if(recv1 > 0){
      Serial.println(recv1);
      msg1.magic = MAGIC;
      msg1.src = 2;
      msg1.len = recv1;
      Serial.write((uint8_t*)&msg1, msg1.len + 12);
      delay(100);
    }
  }
  

  //sws2.listen();
  sifMsg msg2;
  while(sws2.available() > 0 ){
    int recv2 = sws2.readBytes(msg2.data, 256);
    if(recv2 > 0){
      Serial.println(recv2);
      msg2.magic = MAGIC;
      msg2.src = 3;
      msg2.len = recv2;
      Serial.write((uint8_t*)&msg2, msg2.len + 12);
      delay(100);
    }

  }
  
  delay(100);
}
