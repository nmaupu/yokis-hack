#include "serial/rawFlagCallback.h"
#include "globals.h"

RawFlagCallback::RawFlagCallback(char c, String h) : SerialCallback(c, h) {}

bool RawFlagCallback::commandCallback() {
    FLAG_TOGGLE(FLAG_RAW);
    SerialHelper::displayConfig();
    return true;
}