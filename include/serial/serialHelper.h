
#ifndef __SERIAL_HELPER_H__
#define __SERIAL_HELPER_H__

#include "RF/pairing.h"
#include "serial/serialCallback.h"

#define MAX_NUMBER_OF_COMMANDS 32
#define MAX_COMMAND_LENGTH 32
#define MAX_COMMAND_FULL_LENGTH 256

class SerialHelper {
   private:
    SerialCallback* callbacks[MAX_NUMBER_OF_COMMANDS];
    uint8_t callbacksIndex;
    char currentCommand[MAX_COMMAND_FULL_LENGTH];
    int currentCommandIdx;
    unsigned int longestCommandLength;

   public:
    SerialHelper();
    //~SerialHelper();
    void readFromSerial();
    void usage();
    bool registerCallback(SerialCallback* callback);
    bool executeCallback(const char*);
    void extractCommand(char* buf);
    // Executes a command. Returns true if command is found, false otherwise
    bool commandCallback(const char*);

    static bool displayConfig();
    static void prompt();
};

#endif  // __SERIAL_HELPER_H__
