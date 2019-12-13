#include "serial/serialHelper.h"
#include "serial/debugFlagCallback.h"
#include "serial/usageCallback.h"
#include "serial/rawFlagCallback.h"
#include "serial/displayConfigCallback.h"

SerialHelper::SerialHelper() {
    Serial.begin(SERIAL_BAUDRATE);
    ConfigFlags = 0;
    callbacksIndex = 0;

    this->registerCallback(
        new UsageCallback('h', "display this (h)elp", this));
    this->registerCallback(
        new DebugFlagCallback('d', "toggle (d)ebug mode"));
    this->registerCallback(
        new DisplayConfigCallback('p', "(p)rint current config flags"));
    this->registerCallback(
        new RawFlagCallback('r', "toggle (r)aw / formatted output"));
}

bool SerialHelper::registerCallback(SerialCallback* callback) {
    if (callbacksIndex >= MAX_NUMBER_OF_COMMANDS) return false;
    callbacks[callbacksIndex++] = callback;
    return true;
}

void SerialHelper::readFromSerial(void) {
    uint8_t serialIn;

    if (Serial.available() > 0) {
        serialIn = Serial.read();
        for(uint8_t i=0; i<callbacksIndex; i++) {
            if(callbacks[i]->getCommand() == serialIn) {
                callbacks[i]->commandCallback();
            }
        }
    }
}

bool SerialHelper::displayConfig(void) {
    Serial.print("Config:");
    Serial.print(" DEBUG=");
    Serial.print(FLAG_IS_ENABLED(FLAG_DEBUG));
    Serial.print(" RAW=");
    Serial.print(FLAG_IS_ENABLED(FLAG_RAW));
    Serial.println();
    return true;
}

void SerialHelper::usage() {
    Serial.println(PROG_TITLE);
    Serial.println();
    for(uint8_t i=0; i<callbacksIndex; i++) {
        Serial.print(callbacks[i]->getCommand());
        Serial.print("  ");
        Serial.println(callbacks[i]->getHelp());
    }
}
