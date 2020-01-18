#include "RF/copy.h"

Copy::Copy(uint16_t cepin, uint16_t cspin, Device* device)
    : Pairing(cepin, cspin) {
    setDevice(device);
    reset();
}

Copy::Copy(uint16_t cepin, uint16_t cspin) : Copy(cepin, cspin, NULL) {}

void Copy::setDevice(Device* device) { this->device = device; }

void Copy::setupRFModule() {
    setCRCLength(RF24_CRC_16);
    setPALevel(RF24_PA_LOW);
    setChannel(PAIRING_CHANNEL_NUMBER);
    setAutoAck(true);
    setAddressWidth(sizeof(Pairing::pairingAddress));
    delay(5);
}

bool Copy::send() {
    if (device == NULL) return false;

    timeout = millis() + 200;

    begin();
    setupRFModule();

    // First payload
    //
    openWritingPipe(pairingAddress);
    stopListening();

    memcpy(recvBuffer, device->getVersion(), FIRST_PAYLOAD_SIZE);
    _debugPrintRecv(recvBuffer, FIRST_PAYLOAD_SIZE);
    setPayloadSize(FIRST_PAYLOAD_SIZE);
    write(recvBuffer, FIRST_PAYLOAD_SIZE);

    memcpy(recvBuffer, device->getHardwareAddress(), 2);
    recvBuffer[2] = device->getChannel();
    memcpy(recvBuffer + 3, device->getSerial(), 2);
    _debugPrintRecv(recvBuffer, SECOND_PAYLOAD_SIZE);
    setPayloadSize(SECOND_PAYLOAD_SIZE);
    write(recvBuffer, SECOND_PAYLOAD_SIZE);

    while (millis() < timeout) {
        delay(1);  // let time to send packets
    }

    /*
    LOG.print(device->getName());
    if (isFailed) {
        LOG.println(" - Copy failed.");
    } else {
        LOG.println(" - Copied successfully.");
    }*/

    return !isFailed;
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Copy::interruptRxReady() {
    // Nothing will be received
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Copy::interruptTxOk() { isFailed = false; }

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void Copy::interruptTxFailed() {
    isFailed = true;
}
