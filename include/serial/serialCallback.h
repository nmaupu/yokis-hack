#ifndef __SERIAL_CALLBACK_H__
#define __SERIAL_CALLBACK_H__

class SerialCallback {
   protected:
    const char* command;
    const char* help;

   public:
    virtual bool commandCallback(const char*) = 0;

    SerialCallback(const char* command, const char* help) {
        this->command = command;
        this->help = help;
    };

    const char* getCommand() { return command; }

    const char* getHelp() { return help; }
};

#endif  // __SERIAL_CALLBACK_H__
