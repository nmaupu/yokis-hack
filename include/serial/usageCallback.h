#ifndef __USAGE_CALLBACK_H__
#define __USAGE_CALLBACK_H__

#include "serial/serialCallback.h"
#include "serial/serialHelper.h"

class UsageCallback : public SerialCallback {
   private:
    SerialHelper* serialHelper;

   public:
    UsageCallback(const char*, const char*, SerialHelper*);
    bool commandCallback(const char*);
};

#endif  // __USAGE_CALLBACK_H__
