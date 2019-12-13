#include <Arduino.h>
#include "RF/pairing.h"
#include "globals.h"
#include "printf.h"
#include "serial/genericCallback.h"
#include "serial/serialHelper.h"

#define CE_PIN 7
#define CSN_PIN 8

// singletons
SerialHelper* mySerial;
RF* pairingRF;

bool pairingCallback();

void setup() {
    pairingRF = new RF(CE_PIN, CSN_PIN);
    mySerial = new SerialHelper();
    mySerial->registerCallback(new GenericCallback(
        'P', "(P)air with a Yokis device - basically act as a Yokis remote",
        pairingCallback));

    printf_begin();
    mySerial->usage();
}

void loop() {
    // if(pairingRF->hackPairing()) {
    //    pairingRF->printPairingInfo();
    //}

    mySerial->readFromSerial();
    delay(5);
}

bool pairingCallback() { return pairingRF->hackPairing(); }