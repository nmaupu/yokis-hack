#include "RF/configurator.h"

RFConfigurator::RFConfigurator(uint16_t cepin, uint16_t cspin)
    : RF24_forked(cepin, cspin) {}
