#include "serial/debugFlagCallback.h"
#include "globals.h"
#include "serial/serialHelper.h"

DebugFlagCallback::DebugFlagCallback(char c, String h) : SerialCallback(c, h) {}

bool DebugFlagCallback::commandCallback() {
    FLAG_TOGGLE(FLAG_DEBUG);
    SerialHelper::displayConfig();
    return true;
}