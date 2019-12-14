#include "RF/pairing.h"
#include <Arduino.h>
#include "globals.h"

#define HACK_TIMEOUT 30000

// Constants' declaration
const byte Pairing::pairingAddress[] = {0xbe, 0xbe, 0xbe, 0xbe, 0xbe};

// Functions
Pairing::Pairing(uint16_t cepin, uint16_t cspin)
    : RFConfigurator(cepin, cspin) {
    reset();
}

void Pairing::reset() {
    this->recvBufferAddr = this->recvBuffer;
    memset(this->recvBuffer, 0, 8);
    this->readsCount = 0;
    this->timeout = HACK_TIMEOUT;
}

bool Pairing::hackPairing() {
    reset();
    timeout = millis() + HACK_TIMEOUT;

    if (!FLAG_IS_ENABLED(FLAG_RAW) || FLAG_IS_ENABLED(FLAG_DEBUG)) {
        Serial.println("Hack started, click on the connect button when ready");
    }

    begin();
    setupRFModule();

    if (FLAG_IS_ENABLED(FLAG_DEBUG)) {
        printDetails();
    }

    Serial.print("Waiting... timeout=");
    Serial.println(HACK_TIMEOUT);

    while (millis() < timeout && readsCount < 2) {
        delay(10);
    }

    if (readsCount >= 2) {
        printPairingInfo();
        return true;
    }

    Serial.println("Timeout waiting for data, aborting.");
    return false;
}

void Pairing::setupRFModule() {
    // This has been sniffed from SPI
    setCRCLength(RF24_CRC_16);
    setPALevel(RF24_PA_LOW);
    setChannel(PAIRING_CHANNEL_NUMBER);
    setAutoAck(true);
    setAddressWidth(sizeof(Pairing::pairingAddress));
    prepareForReading(3);
    delay(5);
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Pairing::prepareForReading(uint8_t payloadSize) {
    setPayloadSize(payloadSize);
    openReadingPipe(PAIRING_PIPE_NUM, Pairing::pairingAddress);
    startListening();
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Pairing::interruptTxOk() {
    Serial.println("TX sent interrupt");
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Pairing::interruptRxReady() {
    //Serial.println("Pairing RX received");
    if (available()) {
        readsCount++;
        read(recvBufferAddr, getPayloadSize());
        _debugPrintRecv(recvBufferAddr, getPayloadSize());

        // prepare for next packet which should arrive right after this one
        if (getPayloadSize() == 3) {
            timeout = 1000; // after receiving first packet, next one should be following quickly ...
            recvBufferAddr += getPayloadSize();
            prepareForReading(5);
        }
    }
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Pairing::interruptTxFailed() {
    Serial.println("TX sent failed interrupt");
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Pairing::_debugPrintRecv(byte* recvBuf, uint8_t s) {
    if (!FLAG_IS_ENABLED(FLAG_DEBUG)) return;

    Serial.print("Received data: ");
    for (uint8_t i = 0; i < s; i++) {
        Serial.print(recvBuf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

// Get address on which communication occur with the device after successful
// pairing
void Pairing::getAddressFromRecvData(byte* buf) {
    if (buf == NULL) return;
    buf[0] = recvBuffer[3];
    buf[1] = recvBuffer[4];
    buf[2] = recvBuffer[3];
    buf[3] = recvBuffer[4];
    buf[4] = recvBuffer[4];
}

// Get channel on which communication occur with the device after successful
// pairing
byte Pairing::getChannelFromRecvData() { return recvBuffer[5]; }

// Print pairing information to configured serial
void Pairing::printPairingInfo() {
    if (FLAG_IS_ENABLED(FLAG_RAW))
        _printPairingInfoRaw();
    else
        _printPairingInfoFormat();
}

void Pairing::_printPairingInfoRaw() {
    char buf[32];
    byte addr[5];
    byte channel = getChannelFromRecvData();
    getAddressFromRecvData(addr);

    sprintf(buf, "address=%02x%02x%02x%02x%02x,channel=%02x", addr[0], addr[1],
            addr[2], addr[3], addr[4], channel);
    Serial.println(buf);
}

void Pairing::_printPairingInfoFormat() {
    char buf[32];
    byte addr[5];
    byte channel = getChannelFromRecvData();
    getAddressFromRecvData(addr);

    Serial.println("Here are the info got from the device:");

    Serial.print("  Address: ");
    sprintf(buf, "%02x %02x %02x %02x %02x", addr[0], addr[1], addr[2], addr[3],
            addr[4]);
    Serial.println(buf);

    Serial.print("  Channel: ");
    Serial.println(channel, HEX);
}