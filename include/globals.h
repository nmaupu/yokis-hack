#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#if (MQTT_ENABLED || WEBSERVER_ENABLED) && !WIFI_ENABLED
#error "WIFI_ENABLED must be activated to activate either MQTT_ENABLED or WEBERSERVER_ENABLED"
#endif

#include <Arduino.h>

#ifdef ESP8266
#include <Ticker.h>
#endif  // ESP8266

#include "constants.h"

#include "RF/copy.h"
#include "RF/e2bp.h"
#include "RF/pairing.h"
#include "RF/scanner.h"
#include "serial/serialHelper.h"
#include "serial/mySerial.h"
#if defined(ESP8266) && MQTT_ENABLED
#include "net/mqttHass.h"
#endif

#if WEBSERVER_ENABLED
#include <MycilaWebSerial.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Global config flags
extern byte g_ConfigFlags;
// Global vars for all NRF manipulations
extern SerialHelper* g_serial;
extern Pairing* g_pairingRF;
extern E2bp* g_bp;
extern Scanner* g_scanner;
extern Copy* g_copy;
extern Device* g_currentDevice;

#ifdef ESP8266
extern Device* g_devices[MAX_YOKIS_DEVICES_NUM];
extern uint8 g_nb_devices;
extern Ticker* g_deviceStatusPollers[MAX_YOKIS_DEVICES_NUM];

    #if MQTT_ENABLED
        // Don't update the same device during the MQTT_UPDATE_MILLIS_WINDOW
        // time window !
        // HASS sends sometimes multiple times the same MQTT message in a row...
        // Usually 2 MQTT messages sent in a row are processed very quickly
        // something like 3 to 5 ms
        #define MQTT_UPDATE_MILLIS_WINDOW 100
        extern MqttHass* g_mqtt;
    #endif // MQTT_ENABLED

    // Serial configuration over telnet
    /* extern TelnetSpy g_telnetSpy; */
    extern MySerial g_mySerial;
    #if WEBSERVER_ENABLED
        extern WebSerial g_webSerial;
    #endif
    #define LOG g_mySerial

#else // ESP8266
    #define LOG Serial
#endif // ESP8266

#define FLAG_DEBUG (1 << 0)
#define FLAG_RAW (1 << 1)
#define FLAG_POLLING (1 << 2)

// returns true if debug is enabled
#define IS_DEBUG_ENABLED ((FLAG_IS_ENABLED(FLAG_DEBUG)))
// test if a flag is enabled
#define FLAG_IS_ENABLED(f) (((g_ConfigFlags) & (f)) > 0)
// enable a config flag
#define FLAG_ENABLE(f) ((g_ConfigFlags) |= (f))
// disable a config flag
#define FLAG_DISABLE(f) ((g_ConfigFlags) &= ~(f))
// toggle a config flag
#define FLAG_TOGGLE(f) (FLAG_IS_ENABLED(f) ? FLAG_DISABLE(f) : FLAG_ENABLE(f))

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __GLOBALS_H__
