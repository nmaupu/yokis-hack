#include "RF/pairing.h"
#include <Arduino.h>
#include "globals.h"

#define HACK_TIMEOUT 30000

// Constants declaration
const byte RF::pairingAddress[] = {0xbe, 0xbe, 0xbe, 0xbe, 0xbe};

// Functions
RF::RF(uint16_t cepin, uint16_t cspin) : RFConfigurator(cepin, cspin) {}

void RF::setupRFModule() {
    // This has been sniffed from SPI
    myRF->setCRCLength(RF24_CRC_16);
    myRF->setPALevel(RF24_PA_LOW);
    myRF->setChannel(PAIRING_CHANNEL_NUMBER);
    myRF->setAutoAck(true);
    myRF->setAddressWidth(sizeof(RF::pairingAddress));
}

void RF::prepareForReading(uint8_t s) {
    myRF->setPayloadSize(s);
    myRF->openReadingPipe(PAIRING_PIPE_NUM, RF::pairingAddress);
    myRF->startListening();
    delay(5);  // This delay is important
}

// read 's' bytes if available
// return true if data have been read false otherwise
bool RF::receiveData(byte* recvBuf, uint8_t s) {
    if (myRF->available()) {
        myRF->read(recvBuf, s);
        // myRF->closeReadingPipe(PAIRING_PIPE_NUM);
        // myRF->stopListening();

        // print debug information if needed
        // this->_debugPrintRecv(recvBuf, s);

        return true;
    }

    return false;
}

// This function has to be blocking because timing are very tricky...
// Can't do anything else in between
bool RF::hackPairing() {
    unsigned long now = millis();
    unsigned long timeout = now + HACK_TIMEOUT;

    if (!FLAG_IS_ENABLED(FLAG_RAW) || FLAG_IS_ENABLED(FLAG_DEBUG)) {
        Serial.println("Hack started, click on the connect button when ready");
    }

    this->begin();
    this->setupRFModule();
    this->prepareForReading(3);

    if (FLAG_IS_ENABLED(FLAG_DEBUG)) {
        this->printDetails();
    }

    Serial.print("Waiting... timeout=");
    Serial.println(HACK_TIMEOUT);

    while (now < timeout) {
        now = millis();

        // Timing from one reading to the other are tricky here... We have to be
        // very quick, so no logging, no serial out, nothing...
        if (this->receiveData(recvBuffer1, 3)) {  // Reading useless data first
                                                  // (some command perhaps ?)
                                                  // Keep it for now waiting
                                                  // to know what that means
            this->prepareForReading(5);
            if (this->receiveData(recvBuffer2, 5)) {
                // Success!
                this->_debugPrintRecv(recvBuffer1, 3);
                this->_debugPrintRecv(recvBuffer2, 5);
                this->printPairingInfo();
                return true;
            } else {
                // If we are here, receiving info has failed :(
                // Mostly because there was a too long delay between the 2 reads
                // Don't know why though ...
                Serial.println("Receiving has failed, aborting.");
                return false;
            }
        }
        delay(5); // 5 seems to be a good timing for optimal reception and hack to work correctly
    }

    Serial.println("Timeout waiting for data, aborting.");
    return false;
}

void RF::_debugPrintRecv(byte* recvBuf, uint8_t s) {
    if (!FLAG_IS_ENABLED(FLAG_DEBUG)) return;

    Serial.print("Received data: ");
    for (uint8_t i = 0; i < s; i++) {
        Serial.print(recvBuf[i], HEX);
        Serial.print(" ");
    }
    delay(10);
    Serial.println();
}

// Get address on which communication occur with the device after successful
// pairing
void RF::getAddressFromRecvData(byte* buf) {
    if (buf == NULL) return;
    buf[0] = this->recvBuffer2[0];
    buf[1] = this->recvBuffer2[1];
    buf[2] = this->recvBuffer2[0];
    buf[3] = this->recvBuffer2[1];
    buf[4] = this->recvBuffer2[1];
}

// Get channel on which communication occur with the device after successful
// pairing
byte RF::getChannelFromRecvData() { return this->recvBuffer2[2]; }

// Print pairing information to configured serial
void RF::printPairingInfo() {
    if (FLAG_IS_ENABLED(FLAG_RAW))
        this->_printPairingInfoRaw();
    else
        this->_printPairingInfoFormat();
}

void RF::_printPairingInfoRaw() {
    char buf[32];
    byte addr[5];
    byte channel = this->getChannelFromRecvData();
    this->getAddressFromRecvData(addr);

    sprintf(buf, "address=%02x%02x%02x%02x%02x,channel=%02x", addr[0], addr[1],
            addr[2], addr[3], addr[4], channel);
    Serial.println(buf);
}

void RF::_printPairingInfoFormat() {
    char buf[32];
    byte addr[5];
    byte channel = this->getChannelFromRecvData();
    this->getAddressFromRecvData(addr);

    Serial.println("Here are the info got from the device:");

    Serial.print("  Address: ");
    sprintf(buf, "%02x %02x %02x %02x %02x", addr[0], addr[1], addr[2], addr[3],
            addr[4]);
    Serial.println(buf);

    Serial.print("  Channel: ");
    Serial.println(channel, HEX);
}
