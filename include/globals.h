#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <Arduino.h>
#include "RF/e2bp.h"
#include "RF/pairing.h"
#include "RF/scanner.h"
#include "serial/serialHelper.h"
#ifdef ESP8266
#include "net/mqtt.h"
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
#define CE_PIN D4
#define CSN_PIN D8
#define IRQ_PIN D1
#else
// Arduino
#define CE_PIN 7
#define CSN_PIN 8
#define IRQ_PIN 20
#endif

// Define program title
#define PROG_TITLE "-== Yokis hacks v1.0 ==-"

// Serial baudrate
#define SERIAL_BAUDRATE 115200

// Serial print buffer size
#define PRINT_BUFFER_SIZE 32

// Global config flags
extern byte g_ConfigFlags;
// Global vars for all NRF manipulations
extern SerialHelper* g_serial;
extern Pairing* g_pairingRF;
extern E2bp* g_bp;
extern Scanner* g_scanner;
#ifdef ESP8266
extern Mqtt* g_mqtt;
#endif

#define FLAG_DEBUG (1 << 0)
#define FLAG_RAW   (1 << 1)

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

#define MQTT_TOPIC_COMMAND "cmnd"

#define YOKIS_BEGIN_PACKET 0x35
#define YOKIS_END_PACKET 0x53

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __GLOBALS_H__