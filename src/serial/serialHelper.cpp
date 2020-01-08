#include "serial/serialHelper.h"
#include "serial/displayConfigCallback.h"
#include "serial/flagCallback.h"
#include "serial/usageCallback.h"

SerialHelper::SerialHelper() {
    Serial.begin(SERIAL_BAUDRATE);
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
    char buf[256];
    char currentChar;

    if(currentCommandIdx >= MAX_COMMAND_FULL_LENGTH) {
        Serial.println();
        Serial.println("Command too long. Aborting.");
        while (Serial.available()) Serial.read(); // empty serial buffer
        currentCommandIdx = 0;
        prompt();
    }

    if (Serial.available() > 0) {
        currentChar = Serial.read();
        //Serial.println(currentChar, HEX);
        if (currentChar != 0x0a && currentChar != 0x0d) { // \n or \r
            switch (currentChar) {
                case 0x08: // BS
                case 0x7f: // DEL
                    if(currentCommandIdx > 0) {
                        currentCommandIdx--;
                        Serial.write(0x08);
                        Serial.write(' ');
                        Serial.write(0x08);
                    }
                    break;
                default:
                    currentCommand[currentCommandIdx++] = currentChar;
                    Serial.print(currentChar);
            }

            Serial.flush();
        } else if (currentChar == 0x0d) { // \r
            // we ignore \n, some TTY send them, some don't but we don't want to process twice if both are sent
            currentCommand[currentCommandIdx] = '\0';
            Serial.println();

            // Looking for a command to execute
            char extractedCommand[32];
            extractCommand(extractedCommand);

            bool found = executeCallback(extractedCommand);
            if (!found) {
                sprintf(buf, "%s: command not found", extractedCommand);
                Serial.println(buf);
            }

            Serial.println();
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
    Serial.println(PROG_TITLE);
    Serial.println();
    for (uint8_t i = 0; i < callbacksIndex; i++) {
        Serial.print(callbacks[i]->getCommand());
        unsigned int nbSpaces =
            longestCommandLength - strlen(callbacks[i]->getCommand());
        for (uint8_t j = 0; j < nbSpaces + 1; j++) Serial.print(" ");
        Serial.println(callbacks[i]->getHelp());
    }
}

bool SerialHelper::displayConfig(void) {
    Serial.print("Config:");
    Serial.print(" DEBUG=");
    Serial.print(IS_DEBUG_ENABLED);
    Serial.print(" RAW=");
    Serial.print(FLAG_IS_ENABLED(FLAG_RAW));
    Serial.print(" POLLING=");
    Serial.print(FLAG_IS_ENABLED(FLAG_POLLING));
    Serial.println();
    return true;
}

void SerialHelper::prompt() {
    Serial.print("> ");
}
