#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <Arduino.h>

#ifdef ESP8266
#include <Ticker.h>
#endif  // ESP8266

#include "RF/copy.h"
#include "RF/e2bp.h"
#include "RF/pairing.h"
#include "RF/scanner.h"
#include "serial/serialHelper.h"
#if defined(ESP8266) && defined(MQTT_ENABLED)
#include "net/mqttHass.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// CE  active low controls when chips actually executes stuff (sends a payload
// for instance)
// CSN active low controls when commands are sent to the chip (write register,
// etc.)
// Interrupt pin from the NRF chip
#ifdef ESP8266

#define CE_PIN D2
#define CSN_PIN D8
#define IRQ_PIN D1

// status led to turn off
#define STATUS_LED D4

#else

// Arduino
#define CE_PIN 7
#define CSN_PIN 8
#define IRQ_PIN 20

#endif // ESP8266


#define CURRENT_DEVICE_DEFAULT_NAME "tempDevice"

// Define program title
#define PROG_TITLE_FORMAT "-== Yokis hacks v. %s ==-"
#ifndef PROG_VERSION
#define PROG_VERSION "dev"
#endif // PROG_VERSION

// Serial baudrate
#define SERIAL_BAUDRATE 115200

// Serial print buffer size
#define PRINT_BUFFER_SIZE 32

// Yokis commands
#define YOKIS_CMD_BEGIN 0x35
#define YOKIS_CMD_END 0x53
#define YOKIS_CMD_STATUS 0x00
#define YOKIS_CMD_ON 0xb9
#define YOKIS_CMD_OFF 0x1a
#define YOKIS_CMD_SHUTTER_OFF 0xfa
#define YOKIS_CMD_SHUTTER_PAUSE 0x1a

#define DEVICE_MAX_FAILED_POLLING_BEFORE_OFFLINE 3

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
extern Device* g_devices[MQTT_MAX_NUM_OF_YOKIS_DEVICES];
extern Ticker* g_deviceStatusPollers[MQTT_MAX_NUM_OF_YOKIS_DEVICES];

#ifdef MQTT_ENABLED
// Don't update the same device during the MQTT_UPDATE_MILLIS_WINDOW
// time window !
// HASS sends sometimes multiple times the same MQTT message in a row...
// Usually 2 MQTT messages sent in a row are processed very quickly
// something like 3 to 5 ms
#define MQTT_UPDATE_MILLIS_WINDOW 100
extern MqttHass* g_mqtt;
#endif // MQTT_ENABLED

// Serial configuration over telnet
extern TelnetSpy g_telnetAndSerial;
#define LOG g_telnetAndSerial

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
