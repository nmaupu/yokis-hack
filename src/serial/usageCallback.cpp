#include "serial/usageCallback.h"

UsageCallback::UsageCallback(char c, String h, SerialHelper* serialHelper) : SerialCallback(c, h) {
    this->serialHelper = serialHelper;
}

bool UsageCallback::commandCallback() {
    this->serialHelper->usage();
    SerialHelper::displayConfig();
    return true;
}