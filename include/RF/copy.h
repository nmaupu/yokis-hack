#ifndef __COPY_H__
#define __COPY_H__

#include "RF/pairing.h"
#include "RF/device.h"

#define FIRST_PAYLOAD_SIZE  3
#define SECOND_PAYLOAD_SIZE 5

// Class to send payload corresponding to a device on the pairing address
// Useful to emulate the "copy button" function describe on the manual
// This is also the same as when a connect button is clicked on the device.
class Copy : public Pairing {
    private:
     Device* device;
     uint16_t writesCount = 0;
     bool isFailed = false;

    protected:
     void setupRFModule();

    public:
     Copy(uint16_t, uint16_t);
     Copy(uint16_t, uint16_t, Device*);
     void setDevice(Device*);
     bool send();
     void interruptTxOk() override;
     void interruptRxReady() override;
     void interruptTxFailed() override;
};

#endif  // __COPY_H__
