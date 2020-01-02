#ifndef __RF_CONFIGURATOR_H__
#define __RF_CONFIGURATOR_H__

#include "RF/RF24_forked.h"

#define DEFAULT_CE_PIN 7
#define DEFAULT_CS_PIN 8

class RFConfigurator : public RF24 {
   protected:
    // Setup RF module so that it can accept incoming packets
    virtual void setupRFModule() = 0;

   public:
    RFConfigurator(uint16_t cepin, uint16_t cspin);
    // Callback for interrupt when TX_DS bit is set
    virtual void interruptTxOk() = 0;
    // Callback for interrupt when RX_DR bit is set
    virtual void interruptRxReady() = 0;
    // Callback for interrupt when MAX_RT bit is set
    virtual void interruptTxFailed() = 0;
};

#endif  // __RF_CONFIGURATOR_H__
