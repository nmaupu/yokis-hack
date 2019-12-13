#ifndef __DISPLAY_CONFIG_CALLBACK_H__
#define __DISPLAY_CONFIG_CALLBACK_H__

#include <Arduino.h>
#include "globals.h"
#include "serial/serialCallback.h"
#include "serial/serialHelper.h"

class DisplayConfigCallback : public SerialCallback {
   public:
    DisplayConfigCallback(char, String);
    bool commandCallback();
};

#endif  // __DISPLAY_CONFIG_CALLBACK_H__