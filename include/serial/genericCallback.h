#ifndef __GENERIC_CALLBACK_H__
#define __GENERIC_CALLBACK_H__

#include "serial/serialCallback.h"

class GenericCallback : public SerialCallback {
   private:
    bool (*callback)(const char*);

   public:
    GenericCallback(const char*, const char*, bool (*)(const char*));
    bool commandCallback(const char*);
};

#endif
