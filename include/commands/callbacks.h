#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include <stdint.h>

#ifdef ESP8266
#include <Ticker.h>
#endif  // ESP8266

#include "RF/copy.h"
#include "RF/device.h"
#include "RF/e2bp.h"
#include "RF/irqManager.h"
#include "RF/pairing.h"
#include "RF/scanner.h"
#include "serial/genericCallback.h"
#include "net/wifi.h"

void registerAllCallbacks();

bool changeDeviceState(const char*, bool (E2bp::*)(void));

#ifdef ESP8266
void pollDevice(Device* device);  // Interrupt func
#endif // ESP8266

bool pairingCallback(const char*);
bool onCallback(const char*);
bool offCallback(const char*);
bool toggleCallback(const char*);
bool pauseShutterCallback(const char*);
bool scannerCallback(const char*);
bool copyCallback(const char*);
bool displayDevices(const char*);
bool pressCallback(const char*);
bool pressForCallback(const char*);
bool releaseCallback(const char*);
bool statusCallback(const char*);
bool dimmerMemCallback(const char*);
bool dimmerMaxCallback(const char*);
bool dimmerMidCallback(const char*);
bool dimmerMinCallback(const char*);
bool dimmerNilCallback(const char*);
bool dimmerSet(const char*, const uint8_t);

#ifdef ESP8266
Device* getDeviceFromParams(const char*);

bool storeConfigCallback(const char*);
bool clearConfig(const char*);
bool displayConfig(const char*);
bool restoreConfig(const char*);
bool reloadConfig(const char*);
bool deleteFromConfig(const char*);
bool resetWifiConfigCallback(const char*);
bool wifiConfig(const char*);
bool wifiReconnect(const char*);
bool wifiDiag(const char*);
bool restart(const char*);

#if defined(MQTT_ENABLED)
bool mqttConfig(const char*);
bool mqttDiag(const char*);
bool mqttConfigDelete(const char*);
#endif  // MQTT_ENABLED

#endif  // ESP8266

#endif  //__CALLBACKS_H__
