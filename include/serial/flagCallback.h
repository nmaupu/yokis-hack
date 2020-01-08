#ifndef __FLAG_CALLBACK_H__
#define __FLAG_CALLBACK_H__

#include "globals.h"
#include "serial/serialCallback.h"
#include "serial/serialHelper.h"

class FlagCallback : public SerialCallback {
   private:
    uint8_t flag;

   public:
    FlagCallback(const char*, const char*, uint8_t);
    bool commandCallback(const char*);
};

#endif  // __FLAG_CALLBACK_H__
