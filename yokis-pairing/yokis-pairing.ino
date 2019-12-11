#include <RF24.h>
#include <nRF24L01.h>
#include "printf.h"

#define CE_PIN 7
#define CSN_PIN 8
#define BUFFER_SIZE 32
#define RECV_BUFFER_SIZE 5

// Info found by sniffing SPI
const byte address[] = {0xBE, 0xBE, 0xBE, 0xBE, 0xBE};
const byte channel = 0x02;

// radio object
RF24 myRF(CE_PIN, CSN_PIN);

byte bufferRecv[RECV_BUFFER_SIZE];
char printBuffer[BUFFER_SIZE];
char serialBuffer[BUFFER_SIZE];
bool newData = false;
uint8_t pipeNum = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n-== Yokis pairing sniffer ==-");
  printf_begin();
  initBuffer();
  myRF.begin();

  // Power down before configuring registers
  //myRF.powerDown();
  // This is the sniffed configuration
  // No reception for now without disabling CRC checking...
  myRF.write_register( NRF_CONFIG, 0b00001111 );
  myRF.write_register( RF_SETUP, 0b00000011 );
  myRF.write_register( RF_CH, 0b00000010 );
  myRF.write_register( EN_RXADDR, 0b00000001 );
  myRF.write_register( EN_AA, 0b00000001 );
  myRF.write_register( RX_ADDR_P0, address, 5 );
  myRF.write_register( TX_ADDR, address, 5 );
  myRF.flush_rx();
  myRF.write_register( NRF_STATUS, 0b01110000 );
  //myRF.ce(HIGH);
  //delay(1);
  //myRF.write_register( RX_PW_P0, 0x03 ); // First we listen for 3 bytes

  //myRF.disableCRC();
  //myRF.setAutoAck(false);
  //myRF.setAddressWidth(5);

  myRF.setPayloadSize(3);
  myRF.openReadingPipe(0, address);
  myRF.startListening();
  delay(5);
  

  /*
  myRF.setChannel(channel);
  myRF.setCRCLength(RF24_CRC_16);
  myRF.disableCRC();
  myRF.setAutoAck(false);
  myRF.setDataRate(RF24_1MBPS);
  myRF.setPALevel(RF24_PA_LOW);
  myRF.openReadingPipe(0, address);
  myRF.setAddressWidth(5);
  myRF.powerUp();
  */

  myRF.printDetails();
  Serial.println("OK.");
}

void loop() {
  step1();
}

void initBuffer() {
  memset(bufferRecv, 0, RECV_BUFFER_SIZE);
}

void step1() {
  if(myRF.available()) {
    myRF.read(&bufferRecv, 3);
    newData = true;
    showData(3, true);

    myRF.stopListening();
    myRF.setPayloadSize(5);
    myRF.openReadingPipe(0, address);
    myRF.startListening();
    delay(5);

    /*
    myRF.ce(LOW);
    delay(1);
    myRF.ce(HIGH);
    delay(1);
    myRF.write_register( RX_PW_P0, 0x05 ); // Listening now for 5 bytes
    delay(1); // This delay is important !
    */

    while(true) {
      if(myRF.available()) {
        myRF.read(&bufferRecv, 5);
        newData = true;
        showData(5, false);

        // Compute address and channel
        giveMeInfo();
        break;
      }
    }
  }
  delay(1);
}

void showData(int num, bool reset) {
  if (newData) {
    Serial.print("Data received ");
    for(int i=0; i<num; i++) {
      sprintf(printBuffer, "%02x", bufferRecv[i]);
      Serial.print(printBuffer);
      Serial.print(" ");
    }
    Serial.println();
    newData = false;
    if(reset)
      initBuffer();
  }
}

void giveMeInfo() {
  sprintf(printBuffer, "%02x %02x %02x %02x %02x",
          bufferRecv[0],
          bufferRecv[1],
          bufferRecv[0],
          bufferRecv[1],
          bufferRecv[1]);
  Serial.print("Address: ");
  Serial.println(printBuffer);

  sprintf(printBuffer, "%02x", bufferRecv[2]);
  Serial.print("Channel: ");
  Serial.println(printBuffer);
}
