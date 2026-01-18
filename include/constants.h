#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

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
#elif defined(ESP32)
    #define CE_PIN 4
    #define CSN_PIN 5
    #define IRQ_PIN 2
    // status led to turn off (built-in LED on most ESP32 boards)
    #define STATUS_LED 2
#else
    // Arduino
    #define CE_PIN 7
    #define CSN_PIN 8
    #define IRQ_PIN 20
#endif // ESP8266


#define CURRENT_DEVICE_DEFAULT_NAME "tmp"

// Define program title
#define PROG_TITLE_FORMAT "-== Yokis Hack version %s ==-"
#ifndef PROG_VERSION
#define PROG_VERSION "dev"
#endif // PROG_VERSION

// Max number of yokis devices that can be stored
#define MAX_YOKIS_DEVICES_NUM 64

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
#define YOKIS_CMD_OFF_SHUTTER 0xfa
#define YOKIS_CMD_SHUTTER_PAUSE 0x1a

#define DEVICE_MAX_FAILED_POLLING_BEFORE_OFFLINE 3

#endif
