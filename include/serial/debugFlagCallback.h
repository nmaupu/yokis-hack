#ifndef __DEBUG_FLAG_CALLBACK_H
#define __DEBUG_FLAG_CALLBACK_H

#include "serial/serialCallback.h"

class DebugFlagCallback : public SerialCallback {
   public:
    DebugFlagCallback(const char*, const char*);

    bool commandCallback(const char*);
};

#endif  // __DEBUG_FLAG_CALLBACK_H
