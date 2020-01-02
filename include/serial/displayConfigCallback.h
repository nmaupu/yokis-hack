#ifndef __DISPLAY_CONFIG_CALLBACK_H__
#define __DISPLAY_CONFIG_CALLBACK_H__

#include "globals.h"
#include "serial/serialCallback.h"
#include "serial/serialHelper.h"

class DisplayConfigCallback : public SerialCallback {
   public:
    DisplayConfigCallback(const char*, const char*);
    bool commandCallback(const char*);
};

#endif  // __DISPLAY_CONFIG_CALLBACK_H__
