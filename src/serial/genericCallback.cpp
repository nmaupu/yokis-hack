#include "serial/genericCallback.h"
#include <string.h>

GenericCallback::GenericCallback(const char* command, const char* help, bool (*callback)(const char*))
    : SerialCallback(command, help) {
    this->callback = callback;
}

bool GenericCallback::commandCallback(const char* params) {
    return this->callback(params);
}

// static
char* GenericCallback::getNextParam(char* param) {
    return strtok(param, " ");
}
