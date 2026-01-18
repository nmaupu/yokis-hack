#include "RF/scanner.h"
#include "globals.h"

#define BUFFER_MAX 9

Scanner::Scanner(uint16_t cepin, uint16_t cspin) : E2bp(cepin, cspin) {}

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

#if defined(ESP8266) || defined(ESP32)
IRAM_ATTR
#endif
void Scanner::interruptRxReady() {
    if (available()) {
        read(buf, BUFFER_MAX);
        LOG.print("Received : ");
        for (uint8_t i = 0; i < BUFFER_MAX; i++) {
            LOG.print(buf[i], HEX);
            LOG.print(" ");
        }
        LOG.println();
    }
}

#if defined(ESP8266) || defined(ESP32)
IRAM_ATTR
#endif
void Scanner::interruptTxOk() {}

#if defined(ESP8266) || defined(ESP32)
IRAM_ATTR
#endif
void Scanner::interruptTxFailed() {}
