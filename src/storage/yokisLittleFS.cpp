#ifdef ESP8266

#include "storage/yokisLittleFS.h"

bool YokisLittleFS::initialized = false;

// static
void YokisLittleFS::init() {
    if (!YokisLittleFS::initialized) {
        /*
        LittleFSConfig cfg;
        cfg.setAutoFormat(false);
        LittleFS.setConfig(cfg);
        */
        YokisLittleFS::initialized = LittleFS.begin();
    }
}

#endif // ESP8266
