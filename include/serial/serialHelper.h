#ifndef __SERIAL_HELPER_H__
#define __SERIAL_HELPER_H__

#include <Arduino.h>
#include "RF/pairing.h"
#include "globals.h"
#include "serial/serialCallback.h"

#define MAX_NUMBER_OF_COMMANDS 32

class SerialHelper {
   private:
    SerialCallback* callbacks[MAX_NUMBER_OF_COMMANDS];
    uint8_t callbacksIndex;

   public:
    SerialHelper();
    //~SerialHelper();
    void readFromSerial();
    void usage();
    bool registerCallback(SerialCallback* callback);

    static bool displayConfig();
};

#endif  // __SERIAL_HELPER_H__