#include <RF24.h>
//#include <nRF24L01.h>
#include "printf.h"

#define FLAG_D (1 << 0)
#define FLAG_R (1 << 1)
#define _IS_E(f) (((snifferConfig) & (f)) > 0)
#define _E(f)    ((snifferConfig) |=  (f))
#define _D(f)    ((snifferConfig) &= ~(f))
#define _T(f)    (_IS_E(f) ? _D(f) : _E(f))

#define CE_PIN 7
#define CSN_PIN 8
#define BAUDRATE 115200

// Buffers
#define PRINT_BUFFER_SIZE 32
#define SERIAL_BUFFER_SIZE 5
#define RECV_BUFFER_SIZE   5 // we need 5 bytes at maximum
byte bufferRecv[RECV_BUFFER_SIZE];
char printBuffer[PRINT_BUFFER_SIZE];
char serialBuffer[SERIAL_BUFFER_SIZE];

// Radio info
// Info found by sniffing SPI
#define PIPE_NUM 0
#define CHANNEL  2
const byte pairingAddress[] = {0xBE, 0xBE, 0xBE, 0xBE, 0xBE};

// radio object
RF24 myRF(CE_PIN, CSN_PIN);

// serial commands
byte snifferConfig = 0;

void setup() {
  Serial.begin(BAUDRATE);
  usage();
  printf_begin();
  myRF.begin();
}

void loop() {
  setupForPairing();
  
  prepareForReading(3);
  while(true) {
    // Timing from one reading to the other are tricky here... We have to be quick enough
    if(receiveData(3)) { // Reading useless data first (some command perhaps ?)
      prepareForReading(5);
      while(! receiveData(5)) {
        delay(5);
      }

      // Display recv buffer
      giveMeInfo();
    }

    serialCommand();
    delay(5);
  }
}

void setupForPairing() {
  // This has benen sniffed from SPI
  myRF.setCRCLength(RF24_CRC_16);
  myRF.setPALevel(RF24_PA_LOW);
  myRF.setChannel(CHANNEL);
  myRF.setAutoAck(true);
  myRF.setAddressWidth(sizeof(pairingAddress));
}

void prepareForReading(uint8_t s) {
  myRF.setPayloadSize(s);
  myRF.openReadingPipe(PIPE_NUM, pairingAddress);
  myRF.startListening();
  delay(5); // This delay is important
}

// read 's' bytes if available
// return true if data have been read false otherwise
bool receiveData(uint8_t s) {
  if(myRF.available()) {
    myRF.read(&bufferRecv, s);
    myRF.closeReadingPipe(PIPE_NUM);
    myRF.stopListening();
    return true;
  }

  return false;
}

void showData(int num) {
  Serial.print("Data received ");
  for(int i=0; i<num; i++) {
    sprintf(printBuffer, "%02x", bufferRecv[i]);
    Serial.print(printBuffer);
    Serial.print(" ");
  }
  Serial.println();
}

void giveMeInfo() {
  Serial.println("Here are the info scanned from the device:");
  sprintf(printBuffer, "%02x %02x %02x %02x %02x",
          bufferRecv[0],
          bufferRecv[1],
          bufferRecv[0],
          bufferRecv[1],
          bufferRecv[1]);
  Serial.print("  Address: ");
  Serial.println(printBuffer);

  sprintf(printBuffer, "%02x", bufferRecv[2]);
  Serial.print("  Channel: ");
  Serial.println(printBuffer);
}

void displayConfig() {
  sprintf(printBuffer, "DEBUG=%x RAW=%x", _IS_E(FLAG_D), _IS_E(FLAG_R));
  Serial.print("Current config: ");
  Serial.println(printBuffer);
}

void usage() {
  Serial.println();
  Serial.println("-== Yokis pairing sniffer v1.0 ==-");
  Serial.println();
  Serial.println("d  toggle debug mode");
  Serial.println("h  display this help");
  Serial.println("p  print current config flags");
  Serial.println("r  toggle raw / formatted output");
}

void serialCommand() {
  if(Serial.available() > 0) {
    byte incoming = Serial.read();
    if(incoming == 'h') {
      usage();
      displayConfig();
    } else if(incoming == 'd') {
      _T(FLAG_D);
      displayConfig();
    } else if(incoming == 'r') {
      _T(FLAG_R);
      displayConfig();
    } else if(incoming == 'p') {
      displayConfig();
    }
  }
}
