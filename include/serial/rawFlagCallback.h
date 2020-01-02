#ifndef __RAW_FLAG_CALLBACK_H__
#define __RAW_FLAG_CALLBACK_H__

#include "serial/serialCallback.h"
#include "serial/serialHelper.h"
#include "globals.h"

class RawFlagCallback : public SerialCallback {
   public:
    RawFlagCallback(const char*, const char*);
    bool commandCallback(const char*);
};

#endif  // __RAW_FLAG_CALLBACK_H__
