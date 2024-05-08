# STC 

STC (Serial Transmission Control) is a serial protocol.
It was designed to establish communication between a Raspberry Pi and an Arduino Nano.
<br>
This repository contains the source of the protocol.
It is implemented for Linux (#define _LINUX) and Arduino (#define _ARDUINO).
<br>
<br>
Also included is a Sniffer, which can be used to monitor the traffic.

## Concept

The STC protocol sends data in form of packets (messages) similarly to how Ethernet works.
To prevent data loss a packet (SYN) gets resend until a corresponding ACK packets is received (similar to TCP).   

## Structure

A packet can be split into three parts:
* The **magic number**, which is used to determine the start of a packet
* The **header**, which contains control field
* The **body**, which contains the data to be sent

#### Magic Number
```
 0                   1                   2                   3   
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Magic Number                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                                                  (in bits)
```

* Magic Number (0xDECAFBAD): Indicates start of frame

#### Header
``` 
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             ID                |            Length             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                              TCN                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      TCF      |      CRC      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                                                  (in bits)
```

* ID: Indicates type of message and corresponding body
* Length: Length of body in bytes
* TCN(Transmission Control Number): Increments with each message
* TCF(Transmission Control Flags):
    * 0b00000001: SYN
    * 0b00000010: ACK
    * 0b00000100: RST
* CRC: Checksum over complete message except CRC field

## Sniffer
![sniffer0](https://github.com/MarQMark/stc/assets/72945679/44ec305d-0f8a-49db-a8ee-c599a8babe5b)
![sniffer1](https://github.com/MarQMark/stc/assets/72945679/5490b2df-774a-4f0d-ab38-0993c505ff56)
![sniffer2](https://github.com/MarQMark/stc/assets/72945679/2f74e44e-2131-4b34-8416-71d378600269)


