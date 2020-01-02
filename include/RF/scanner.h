#ifndef __SCANNER_H__
#define __SCANNER_H__

#include <Arduino.h>
#include "RF/e2bp.h"

class Scanner : public E2bp {
   private:
    uint8_t buf[32];

   public:
    Scanner(uint16_t, uint16_t);
    void setupRFModule() override;
    void interruptTxOk() override;
    void interruptRxReady() override;
    void interruptTxFailed() override;
};

#endif
