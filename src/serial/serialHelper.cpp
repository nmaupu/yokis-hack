#include "serial/serialHelper.h"
#include "serial/displayConfigCallback.h"
#include "serial/flagCallback.h"
#include "serial/usageCallback.h"

SerialHelper::SerialHelper() {
    LOG.begin(SERIAL_BAUDRATE);
    callbacksIndex = 0;
    longestCommandLength = 0;
    currentCommandIdx = 0;

    this->registerCallback(
        new UsageCallback("help", "display this help", this));
    this->registerCallback(
        new FlagCallback("debug", "toggle debug mode", FLAG_DEBUG));
    this->registerCallback(
        new FlagCallback("raw", "toggle raw / formatted output", FLAG_RAW));
    this->registerCallback(
        new FlagCallback("poll", "toggle devices polling for status", FLAG_POLLING));
    this->registerCallback(
        new DisplayConfigCallback("config", "print current config flags"));
}

bool SerialHelper::registerCallback(SerialCallback* callback) {
    if (callbacksIndex >= MAX_NUMBER_OF_COMMANDS) return false;
    callbacks[callbacksIndex++] = callback;

    if (strlen(callback->getCommand()) > longestCommandLength)
        longestCommandLength = strlen(callback->getCommand());

    return true;
}

void SerialHelper::readFromSerial(void) {
    char buf[MAX_COMMAND_FULL_LENGTH];
    char currentChar;

    if(currentCommandIdx >= MAX_COMMAND_FULL_LENGTH) {
        LOG.println();
        LOG.println("Command too long. Aborting.");
        while (LOG.available()) LOG.read(); // empty serial buffer
        currentCommandIdx = 0;
        prompt();
    }

    if (LOG.available() > 0) {
        currentChar = LOG.read();
        //LOG.println(currentChar, HEX);
        if (currentChar != 0x0a && currentChar != 0x0d) { // \n or \r
            switch (currentChar) {
                case 0x08: // BS
                case 0x7f: // DEL
                    if(currentCommandIdx > 0) {
                        currentCommandIdx--;
                        LOG.write(0x08);
                        LOG.write(' ');
                        LOG.write(0x08);
                    }
                    break;
                default:
                    currentCommand[currentCommandIdx++] = currentChar;
                    LOG.print(currentChar);
            }

            LOG.flush();
        } else if (currentChar == 0x0d) { // \r
            // we ignore \n, some TTY send them, some don't but we don't want to process twice if both are sent
            currentCommand[currentCommandIdx] = '\0';
            LOG.println();

            // Looking for a command to execute
            char extractedCommand[32];
            extractCommand(extractedCommand);

            if (strcmp("", extractedCommand) != 0) {
                bool found = executeCallback(extractedCommand);
                if (!found) {
                    sprintf(buf, "%s: command not found", extractedCommand);
                    LOG.println(buf);
                }
            }

            LOG.println();
            currentCommandIdx = 0;
            prompt();
        }
    } // if
}

bool SerialHelper::executeCallback(const char* cmd) {
    bool found = false;
    for (uint8_t i = 0; i < callbacksIndex; i++) {
        if (strcmp(cmd, callbacks[i]->getCommand()) == 0) {
            found = true;
            callbacks[i]->commandCallback(currentCommand);
            break;
        }
    }

    return found;
}

void SerialHelper::extractCommand(char* buf) {
    uint8_t p = 0;
    while(currentCommand[p] != ' ' && p < MAX_COMMAND_LENGTH && p < currentCommandIdx) {
        buf[p] = currentCommand[p];
        p++;
    }
    buf[p] = '\0';
}

void SerialHelper::usage() {
    char buf[64];
    sprintf(buf, PROG_TITLE_FORMAT, PROG_VERSION);
    LOG.println(buf);
    LOG.println();

    for (uint8_t i = 0; i < callbacksIndex; i++) {
        LOG.print(callbacks[i]->getCommand());
        unsigned int nbSpaces =
            longestCommandLength - strlen(callbacks[i]->getCommand());
        for (uint8_t j = 0; j < nbSpaces + 1; j++) LOG.print(" ");
        LOG.println(callbacks[i]->getHelp());
    }
}

bool SerialHelper::displayConfig(void) {
    LOG.print("Config:");
    LOG.print(" DEBUG=");
    LOG.print(IS_DEBUG_ENABLED);
    LOG.print(" RAW=");
    LOG.print(FLAG_IS_ENABLED(FLAG_RAW));
    LOG.print(" POLLING=");
    LOG.print(FLAG_IS_ENABLED(FLAG_POLLING));
    LOG.println();
    return true;
}

void SerialHelper::prompt() {
    LOG.print("> ");
}
