#include "serialHelper.h"

SerialHelper::SerialHelper() {
    Serial.begin(BAUDRATE);
    config = 0;
}

void SerialHelper::readFromSerial(void) {
    if(Serial.available() > 0) {
        serialIn = Serial.read();

        switch (serialIn) {
            case 'h':
                usage();
                displayConfig();
                break;
            case 'd':
                _TOGGLE(config, FLAG_DEBUG);
                displayConfig();
                break;
            case 'r':
                _TOGGLE(config, FLAG_RAW_OUTPUT);
                displayConfig();
                break;
            case 'p':
                displayConfig();
                break;
            default:
                break;
        }
    }
}

void SerialHelper::usage() {
    Serial.println();
    Serial.println(SerialHelper::TITLE);
    Serial.println();
    Serial.println("d  toggle debug mode");
    Serial.println("h  display this help");
    Serial.println("p  print current config flags");
    Serial.println("r  toggle raw / formatted output");
}

void SerialHelper::displayConfig(void) {
    sprintf(printBuffer, 
        "DEBUG=%x RAW=%x", 
        _IS_ENABLED(config, FLAG_DEBUG), 
        _IS_ENABLED(config, FLAG_RAW_OUTPUT));
    Serial.print("Current config: ");
    Serial.println(printBuffer);
}