#ifdef ESP8266
#ifndef __YOKIS_LITTLE_FS_H__
#define __YOKIS_LITTLE_FS_H__

#include <LittleFS.h>

class YokisLittleFS {
    private:
     static bool initialized;

    public:
     static void init();
};

#endif  // __YOKIS_LITTLE_FS_H__
#endif  // ESP8266
