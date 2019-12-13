#ifndef __DEBUG_FLAG_CALLBACK_H
#define __DEBUG_FLAG_CALLBACK_H

#include <Arduino.h>
#include "serial/serialCallback.h"

class DebugFlagCallback : public SerialCallback {
   public:
    DebugFlagCallback(char, String);

    bool commandCallback();
};

#endif  // __DEBUG_FLAG_CALLBACK_H