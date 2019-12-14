#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define program title
#define PROG_TITLE "-== Yokis pairing sniffer v1.0 ==-"

// Serial baudrate
#define SERIAL_BAUDRATE 115200

// Serial print buffer size
#define PRINT_BUFFER_SIZE 32

// Global config flags
extern byte ConfigFlags;

#define FLAG_DEBUG (1 << 0)
#define FLAG_RAW   (1 << 1)

// test if a flag is enabled
#define FLAG_IS_ENABLED(f) ( ((ConfigFlags) & (f)) > 0 )
// test if debug flag is enabled
#define IS_DEBUG_ENABLED (FLAG_IS_ENABLED(FLAG_DEBUG))
// enable a config flag
#define FLAG_ENABLE(f) ( (ConfigFlags) |= (f) )
// disable a config flag
#define FLAG_DISABLE(f) ( (ConfigFlags) &= ~(f) )
// toggle a config flag
#define FLAG_TOGGLE(f) \
    ( FLAG_IS_ENABLED(f) ? FLAG_DISABLE(f) : FLAG_ENABLE(f) )

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __GLOBALS_H__