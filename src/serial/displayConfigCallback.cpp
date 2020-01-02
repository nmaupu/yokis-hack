#include "serial/displayConfigCallback.h"

DisplayConfigCallback::DisplayConfigCallback(const char* command, const char* help)
    : SerialCallback(command, help) {}

bool DisplayConfigCallback::commandCallback(const char* params) {
    SerialHelper::displayConfig();
    return true;
}
