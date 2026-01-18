#if defined(ESP8266) || defined(ESP32)
#ifndef __YOKIS_LITTLE_FS_H__
#define __YOKIS_LITTLE_FS_H__

#include <LittleFS.h>
#define FORMAT_LITTLEFS_IF_FAILED true

class YokisLittleFS {
    private:
     static bool initialized;

    public:
     static void init();
};

#endif  // __YOKIS_LITTLE_FS_H__
#endif  // ESP8266 || ESP32
