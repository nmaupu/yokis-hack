#include "serial/rawFlagCallback.h"
#include "globals.h"

RawFlagCallback::RawFlagCallback(const char* command, const char* help)
    : SerialCallback(command, help) {}

bool RawFlagCallback::commandCallback(const char* params) {
    FLAG_TOGGLE(FLAG_RAW);
    SerialHelper::displayConfig();
    return true;
}
