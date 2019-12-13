#include "RF/configurator.h"

RFConfigurator::RFConfigurator(uint16_t cepin, uint16_t cspin) : RF24(cepin, cspin) {
    myRF = new RF24(cepin, cspin);
}

RFConfigurator::~RFConfigurator() { delete (myRF); }

RF24* RFConfigurator::getRF24() { return this->myRF; }