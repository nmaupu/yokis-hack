#include "RF/pairing.h"
#include <Arduino.h>
#include "globals.h"

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
        LOG.println("Hack started, click on the connect button when ready");
    }

    begin();
    setupRFModule();

    if (FLAG_IS_ENABLED(FLAG_DEBUG)) {
        printDetails();
    }

    LOG.print("Waiting... timeout=");
    LOG.println(HACK_TIMEOUT);

    while (millis() < timeout && this->readsCount < 2) {
        delay(10);
    }

    if (this->readsCount >= 2) {
        printPairingInfo();
        return true;
    }

    LOG.println("Timeout waiting for data, aborting.");
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
    LOG.println("TX sent interrupt");
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Pairing::interruptRxReady() {
    // LOG.println("Pairing RX received");
    if (available()) {
        this->readsCount++;
        read(recvBufferAddr, getPayloadSize());
        _debugPrintRecv(recvBufferAddr, getPayloadSize());

        // prepare for next packet which should arrive right after this one
        if (getPayloadSize() == 3) {
            // after receiving first packet, next one should be following
            // quickly ...
            // millis is ok in interrupts but won't be updated
            // That's ok for this case
            timeout = millis() + 1000;
            recvBufferAddr += getPayloadSize();
            prepareForReading(5);
        }
    }
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Pairing::interruptTxFailed() {
    //LOG.println("TX sent failed interrupt");
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Pairing::_debugPrintRecv(byte* recvBuf, uint8_t s) {
    if (!FLAG_IS_ENABLED(FLAG_DEBUG)) return;

    LOG.print("Buffer data: ");
    for (uint8_t i = 0; i < s; i++) {
        LOG.print(recvBuf[i], HEX);
        LOG.print(" ");
    }
    LOG.println();
}

// Get address on which communication occur with the device after successful
// pairing
void Pairing::getAddressFromRecvData(uint8_t buf[5]) {
    buf[0] = recvBuffer[3];
    buf[1] = recvBuffer[4];
    buf[2] = recvBuffer[3];
    buf[3] = recvBuffer[4];
    buf[4] = recvBuffer[4];
}

// Get channel on which communication occur with the device after successful
// pairing
byte Pairing::getChannelFromRecvData() { return recvBuffer[5]; }

// Get device's version bytes - not sure what those bytes are for yet
void Pairing::getVersionFromRecvData(uint8_t buf[3]) {
    memcpy(buf, recvBuffer, 3);
}

// Get device's serial - not sure what those bytes are for yet
void Pairing::getSerialFromRecvData(uint8_t buf[2]) {
    memcpy(buf, recvBuffer+sizeof(uint8_t)*6, 2);
}

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
    LOG.println(buf);
}

void Pairing::_printPairingInfoFormat() {
    char buf[32];
    byte addr[5];
    byte channel = getChannelFromRecvData();
    getAddressFromRecvData(addr);

    LOG.println("Here are the info got from the device:");

    LOG.print("  Address: ");
    sprintf(buf, "%02x %02x %02x %02x %02x", addr[0], addr[1], addr[2], addr[3],
            addr[4]);
    LOG.println(buf);

    LOG.print("  Channel: ");
    LOG.println(channel, HEX);
}
