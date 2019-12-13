#include "serial/genericCallback.h"

GenericCallback::GenericCallback(char c, String h, bool (*callback)(void)) : SerialCallback(c, h) {
    this->callback = callback;  
}

bool GenericCallback::commandCallback() {
    return this->callback();
}
