#if defined(ESP8266) || defined(ESP32)
#ifndef __YOKIS_LITTLE_FS_H__
#define __YOKIS_LITTLE_FS_H__

#if defined(ESP8266) || defined(ESP32)
#include <LittleFS.h>
#endif

class YokisLittleFS {
    private:
     static bool initialized;

    public:
     static void init();
};

#endif  // __YOKIS_LITTLE_FS_H__
#endif  // ESP8266 || ESP32
