#include "serial/displayConfigCallback.h"

DisplayConfigCallback::DisplayConfigCallback(char c, String h) : SerialCallback(c, h) {}

bool DisplayConfigCallback::commandCallback() {
    SerialHelper::displayConfig();
    return true;
}