#if defined(ESP8266) || defined(ESP32)

#include "storage/yokisLittleFS.h"
#include "globals.h"

bool YokisLittleFS::initialized = false;

// static
void YokisLittleFS::init() {
    if (!YokisLittleFS::initialized) {
        /*
        LittleFSConfig cfg;
        cfg.setAutoFormat(false);
        LittleFS.setConfig(cfg);
        */
        YokisLittleFS::initialized = LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED);
        if (!YokisLittleFS::initialized) {
          LOG.println("LittleFS mount failed");
          return;
        }
    }
}

#endif // ESP8266 || ESP32
