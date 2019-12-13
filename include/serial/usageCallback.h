#ifndef __USAGE_CALLBACK_H__
#define __USAGE_CALLBACK_H__

#include <Arduino.h>
#include "serial/serialCallback.h"
#include "serial/serialHelper.h"

class UsageCallback : public SerialCallback {
   private:
    SerialHelper* serialHelper;

   public:
    UsageCallback(char, String, SerialHelper*);
    bool commandCallback();
};

#endif  // __USAGE_CALLBACK_H__