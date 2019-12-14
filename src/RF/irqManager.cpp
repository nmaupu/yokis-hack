#include "RF/irqManager.h"
#include "RF/configurator.h"
#include "globals.h"

// Needed to compile on ESP8266 platform
// To use interrupt, we have to use ICACHE_RAM_ATTR to store the function in the
// RAM See
// https://stackoverflow.com/questions/58113937/esp8266-arduino-why-is-it-necessary-to-add-the-icache-ram-attr-macro-to-isrs-an
#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
// Process IRQ - function has to be static for Arduino's attachInterrupt
// I never find another way of doing this than by using IRQ pin :/
// The guys from Yokis made this complicated as far as I can tell...
// I would be very curious to see their code but hey, they support a whole bunch
// of NRF24 chips !
void IrqManager::processIRQ() {
    // Any RFConfigurator object can do the job
    RFConfigurator* rf = NULL;
    bool txOk, txFailed, rxReady;

    switch (IrqManager::irqType) {
        case PAIRING:
            rf = g_pairingRF;
            break;
        case E2BP:
            rf = g_bp;
            break;
        case SCANNER:
            rf = g_scanner;
            break;
    }

    rf->whatHappened(txOk, txFailed, rxReady);
    if (txOk)     rf->interruptTxOk();
    if (txFailed) rf->interruptTxFailed();
    if (rxReady)  rf->interruptRxReady();
}
