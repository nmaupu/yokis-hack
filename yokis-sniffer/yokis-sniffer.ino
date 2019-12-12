#include <RF24.h>
#include "printf.h"

#define CE_PIN 7
#define CSN_PIN 8
#define BUFFER_SIZE 32

// Info found by sniffing SPI
const byte address[6] = {0x76, 0x89, 0x76, 0x89, 0x89};
const byte channel = 0x3D;

RF24 myRF24(CE_PIN, CSN_PIN);

byte bufferRecv[BUFFER_SIZE];
char printBuffer[2];
bool newData = false;

void setup() {
  uint8_t i;
  Serial.begin(115200);
  Serial.println("\n-== Yokis sniffer ==-");
  printf_begin();
  initBuffer();
  myRF24.begin();
  myRF24.setChannel(channel);
  myRF24.disableCRC();
  myRF24.setAutoAck(false);
  myRF24.setDataRate(RF24_250KBPS);
  myRF24.setPALevel(RF24_PA_LOW);
  myRF24.openReadingPipe(0, address);
  myRF24.setAddressWidth(5);
  myRF24.printDetails();
  myRF24.startListening();
  Serial.println("OK.");
}

void loop() {
  getData();
}

void initBuffer() {
  memset(bufferRecv, 0, BUFFER_SIZE);
}

void getData() {
  if(myRF24.available()) {
    myRF24.read(&bufferRecv, BUFFER_SIZE);
    newData = true;
    showData();
  }
}

void showData() {
  if (newData) {
    Serial.print("Data received ");
    for(int i=0; i<BUFFER_SIZE; i++) {
      sprintf(printBuffer, "%02x", bufferRecv[i]);
      Serial.print(printBuffer);
      Serial.print(" ");
    }
    Serial.println();
    newData = false;
    initBuffer();
  }
}
