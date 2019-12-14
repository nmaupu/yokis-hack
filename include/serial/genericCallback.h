#ifndef __GENERIC_CALLBACK_H__
#define __GENERIC_CALLBACK_H__

#include "serial/serialCallback.h"

class GenericCallback : public SerialCallback {
   private:
    bool (*callback)(const char*);

   public:
    GenericCallback(const char*, const char*, bool (*)(const char*));
    bool commandCallback(const char*);
    // Get next parameter from a char* given in parameter. The parameter is
    // modified and used to return the next token.
    // Like strtok, if NULL is passed, the next param from the previous
    // call is returned.
    // The function returned a pointer to the next available token.
    // If no more token is available, NULL is returned.
    static char* getNextParam(char*);
};

#endif
