#ifndef __SERIAL_CALLBACK_H__
#define __SERIAL_CALLBACK_H__

#include <Arduino.h>

class SerialCallback {
   protected:
    char command;
    String help;

   public:
    virtual bool commandCallback(void) = 0;

    SerialCallback(char command, String help) {
        this->command = command;
        this->help = help;
    };

    char getCommand() { return command; }
    
    String getHelp() { return help; }
};

#endif  // __SERIAL_CALLBACK_H__