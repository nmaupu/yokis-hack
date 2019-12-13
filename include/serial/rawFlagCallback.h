#ifndef __RAW_FLAG_CALLBACK_H__
#define __RAW_FLAG_CALLBACK_H__

#include <Arduino.h>
#include "serial/serialCallback.h"
#include "serial/serialHelper.h"
#include "globals.h"

class RawFlagCallback : public SerialCallback {
   public:
    RawFlagCallback(char, String);
    bool commandCallback();
};

#endif  // __RAW_FLAG_CALLBACK_H__