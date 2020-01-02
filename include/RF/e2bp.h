#ifndef __E2BP_H__
#define __E2BP_H__

#include <Arduino.h>
#include <nRF24L01.h>
#include "RF/configurator.h"
#include "RF/device.h"
#include "RF/RF24_forked.h"

#define PAYLOAD_LENGTH 9
#define MAIN_LOOP_TIMEOUT_MILLIS 200

enum DeviceStatus { OFF, ON, UNDEFINED };

// Very low level class to control a Yokis device just like a e2bp remote
class E2bp : public RFConfigurator {
   private:
    Device* device;
    bool loopContinue;
    DeviceStatus firstPayloadStatus, secondPayloadStatus;
    char currentPayload[PAYLOAD_LENGTH];

    bool setDeviceStatus(DeviceStatus);
    void reset();
    void getFirstPayload(uint8_t *);
    void getSecondPayload(uint8_t *);

   protected:
    void runMainLoop();
    void setupPayload(const uint8_t *);

   public:
    uint8_t counter = 0;
    // Constructor
    E2bp(uint16_t, uint16_t);
    ~E2bp();
    bool sendPayload(const uint8_t*);
    void stopMainLoop();
    bool on();
    bool off();
    bool toggle();
    void setupRFModule() override;
    void interruptTxOk() override;
    void interruptRxReady() override;
    void interruptTxFailed() override;

    // Getters / setters
    void setDevice(const Device*);
    const Device* getDevice();
};

#endif  // __E2BP_H__
