#ifndef __SERIAL_HELPER_H__
#define __SERIAL_HELPER_H__

#include <Arduino.h>

#define BAUDRATE 115200

#define PRINT_BUFFER_SIZE 32
#define SERIAL_BUFFER_SIZE 5

// test if a flag is enabled
#define _IS_ENABLED(c,f) (((c) & (f)) > 0)
// enable a config flag
#define _ENABLE(c,f) ((c) |= (f))
// disable a config flag
#define _DISABLE(c,f) ((c) &= ~(f))
// toggle a config flag
#define _TOGGLE(c,f) (_IS_ENABLED(c,f) ? _DISABLE(c,f) : _ENABLE(c,f))

// Options FLAGS
#define FLAG_DEBUG      (1 << 0)
#define FLAG_RAW_OUTPUT (1 << 1)

class SerialHelper {
private:
    const String TITLE = "-== Yokis pairing sniffer v1.0 ==-";

    char printBuffer[PRINT_BUFFER_SIZE];
    char serialIn;
    // config byte
    byte config;

public:
    SerialHelper();
    //~SerialHelper();
    void readFromSerial();
    void usage();
    void displayConfig();
};

#endif  // __SERIAL_HELPER_H__