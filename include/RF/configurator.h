#ifndef __RF_CONFIGURATOR_H__
#define __RF_CONFIGURATOR_H__

#include <RF24.h>

#define DEFAULT_CE_PIN 7
#define DEFAULT_CS_PIN 8

class RFConfigurator : public RF24 {
   protected:
    RF24* myRF;
    virtual void setupRFModule() = 0;

   public:
    RFConfigurator(uint16_t cepin, uint16_t cspin);
    ~RFConfigurator();
    RF24* getRF24();
};

#endif  // __RF_CONFIGURATOR_H__