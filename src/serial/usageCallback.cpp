#include "serial/usageCallback.h"

UsageCallback::UsageCallback(const char* command, const char* help, SerialHelper* serialHelper)
    : SerialCallback(command, help) {
    this->serialHelper = serialHelper;
}

bool UsageCallback::commandCallback(const char* params) {
    this->serialHelper->usage();
    Serial.println();
    SerialHelper::displayConfig();
    return true;
}
