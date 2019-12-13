#ifndef __PAIRING_RF_H__
#define __PAIRING_RF_H__

#include <Arduino.h>
#include <RF24.h>
#include "RF/configurator.h"
#include "serial/serialHelper.h"

#define PAIRING_RECV_BUFFER_SIZE_1 3
#define PAIRING_RECV_BUFFER_SIZE_2 5
#define PAIRING_PIPE_NUM 0
#define PAIRING_CHANNEL_NUMBER 2

class RF : public RFConfigurator {
   private:
    // First reception is 3 bytes - don't know yet their meaning
    byte recvBuffer1[PAIRING_RECV_BUFFER_SIZE_1];
    // Second reception is 5 bytes - address + channel + unknown 2 bytes as of yet
    byte recvBuffer2[PAIRING_RECV_BUFFER_SIZE_2];
    static const byte pairingAddress[5];
    void _debugPrintRecv(byte*, uint8_t);
    void _printPairingInfoRaw();
    void _printPairingInfoFormat();

   protected:
    // Setup RF module to get ready to receive data
    void prepareForReading(uint8_t);
    // Actually try to receive data
    bool receiveData(byte*, uint8_t);

   public:
    // Constructor
    RF(uint16_t, uint16_t);
    // Setup RF module so that it can accept incoming packets
    void setupRFModule();
    // Try to receive info from Yokis device (after click on connect button)
    bool hackPairing();
    // Get device address from received data
    void getAddressFromRecvData(byte*);
    // Get device channel from received data
    byte getChannelFromRecvData();
    // Serial print information received
    void printPairingInfo();
};

#endif