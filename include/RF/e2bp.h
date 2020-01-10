#ifndef __E2BP_H__
#define __E2BP_H__

#include <Arduino.h>
#include <nRF24L01.h>
#include "RF/RF24_forked.h"
#include "RF/configurator.h"
#include "RF/device.h"

#define PAYLOAD_LENGTH 9
#define MAIN_LOOP_TIMEOUT_MILLIS 500

// Very low level class to control a Yokis device just like a e2bp remote
class E2bp : public RFConfigurator {
   private:
    Device *device;
    bool loopContinue;
    DeviceStatus firstPayloadStatus, secondPayloadStatus;

    bool setDeviceStatus(DeviceStatus);
    void getFirstPayload(uint8_t *);
    void getSecondPayload(uint8_t *);
    void getStatusPayload(uint8_t *);
    void getOnPayload(uint8_t *);
    void getOffPayload(uint8_t *);
    void getPayload(uint8_t *, DeviceStatus);

   protected:
    bool runMainLoop();
    void setupPayload(const uint8_t *);

   public:
    //uint8_t counter = 0;
    // Constructor
    E2bp(uint16_t, uint16_t);
    ~E2bp();
    bool sendPayload(const uint8_t *);
    void stopMainLoop();
    bool on();
    bool off();
    bool toggle();
    // See Yokis MTV500ER manual for those configs
    // Note: depending on configuration, 2 pulses can set to "memory" or 100%
    // default is 100% for 2 pulses, that's what we will be using here...
    bool dimmerSet(const uint8_t);
    // Set dimmer to "mem" Light
    bool dimmerMem();
    // Set dimmer to max light
    bool dimmerMax();
    // Set dimmer to middle light
    bool dimmerMid();
    // Set dimmer to min light
    bool dimmerMin();
    // Set dimmer to "night light"
    bool dimmerNiL();
    // press the button
    bool press();
    // release the button
    bool release();
    void reset();
    void setupRFModule() override;
    DeviceStatus getLastKnownDeviceStatus();
    // Get the device status (ON or OFF) - experimental !
    DeviceStatus pollForStatus();
    void interruptTxOk() override;
    void interruptRxReady() override;
    void interruptTxFailed() override;


    // Getters / setters
    void setDevice(Device *);
    const Device *getDevice();
};

#endif  // __E2BP_H__
