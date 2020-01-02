#include "RF/scanner.h"

#define BUFFER_MAX 9

Scanner::Scanner(uint16_t cepin, uint16_t cspin) : E2bp(cepin, cspin) {}
Scanner::Scanner(uint16_t cepin, uint16_t cspin, Device* device)
    : E2bp(cepin, cspin, device){};

void Scanner::setupRFModule() {
    if (getDevice() == NULL || getDevice()->getHardwareAddress() == NULL) return;

    begin();
    disableCRC();
    setPayloadSize(BUFFER_MAX);
    setAutoAck(false);
    setDataRate(RF24_250KBPS);
    setPALevel(RF24_PA_HIGH);
    setChannel(getDevice()->getChannel());
    openReadingPipe(0, getDevice()->getHardwareAddress());
    startListening();
    printDetails();
    delay(5);
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Scanner::interruptRxReady() {
    if (available()) {
        read(buf, BUFFER_MAX);
        Serial.print("Received : ");
        for (uint8_t i = 0; i < BUFFER_MAX; i++) {
            Serial.print(buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Scanner::interruptTxOk() {}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Scanner::interruptTxFailed() {}
