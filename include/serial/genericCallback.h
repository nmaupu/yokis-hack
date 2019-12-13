#ifndef __GENERIC_CALLBACK_H__
#define __GENERIC_CALLBACK_H__

#include <Arduino.h>
#include "serial/serialCallback.h"

class GenericCallback : public SerialCallback {
    private:
      bool (*callback)(void);

    public:
     GenericCallback(char, String, bool (*)(void));
     bool commandCallback();
};

#endif