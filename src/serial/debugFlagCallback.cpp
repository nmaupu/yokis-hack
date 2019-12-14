#include "serial/debugFlagCallback.h"
#include "globals.h"
#include "serial/serialHelper.h"

DebugFlagCallback::DebugFlagCallback(const char* command, const char* help)
    : SerialCallback(command, help) {}

bool DebugFlagCallback::commandCallback(const char* params) {
    FLAG_TOGGLE(FLAG_DEBUG);
    SerialHelper::displayConfig();
    return true;
}
