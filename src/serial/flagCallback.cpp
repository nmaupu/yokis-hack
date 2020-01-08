#include "serial/flagCallback.h"
#include "globals.h"

FlagCallback::FlagCallback(const char* command, const char* help,
                           uint8_t flag)
    : SerialCallback(command, help) {
    this->flag = flag;
}

bool FlagCallback::commandCallback(const char*) {
    FLAG_TOGGLE(flag);
    SerialHelper::displayConfig();
    return true;
}
