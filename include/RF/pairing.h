#ifndef __PAIRING_RF_H__
#define __PAIRING_RF_H__

#include <Arduino.h>
#include <RF24.h>
#include <nRF24L01.h>
#include "RF/configurator.h"

#define PAIRING_RECV_BUFFER_SIZE_1 8
#define PAIRING_RECV_BUFFER_SIZE_2 5
#define PAIRING_PIPE_NUM 0
#define PAIRING_CHANNEL_NUMBER 2

class Pairing : public RFConfigurator {
   private:
    uint8_t recvBuffer[8];
    uint8_t* recvBufferAddr;
    uint8_t readsCount;
    static const byte pairingAddress[5];
    void _debugPrintRecv(byte*, uint8_t);
    void _printPairingInfoRaw();
    void _printPairingInfoFormat();
    unsigned long timeout;

   protected:
    // Setup RF module to get ready to receive data
    void prepareForReading(uint8_t);

   public:
    // Constructor
    Pairing(uint16_t, uint16_t);
    // Reinitialize this RF object
    void reset();
    void setupRFModule() override;
    void interruptTxOk() override;
    void interruptRxReady() override;
    void interruptTxFailed() override;
    // Try to receive info from Yokis device (after click on connect button)
    bool hackPairing();
    // Get device address from received data
    void getAddressFromRecvData(uint8_t buf[5]);
    // Get device channel from received data
    byte getChannelFromRecvData();
    // Get device's version bytes - not sure what those bytes are for yet
    void getVersionFromRecvData(uint8_t buf[3]);
    // Get device's serial - not sure what those bytes are for yet
    void getSerialFromRecvData(uint8_t buf[2]);
    // Serial print information received
    void printPairingInfo();
};

#endif
