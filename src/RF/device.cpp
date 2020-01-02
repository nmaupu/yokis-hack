#include "RF/device.h"
#ifdef ESP8266
#include <FS.h>
#endif
#include "globals.h"
#include "utils.h"

#define HARDWARE_ADDRESS_LENGTH 5

#ifdef ESP8266
// Configuration filename to store on SPIFFS when using ESP8266
#define SPIFFS_CONFIG_FILENAME "/yokis.conf"
#define SPIFFS_CONFIG_BAK_FILENAME "/yokis.conf.bak"
#define SPIFFS_CONFIG_SEP "|"
#define SEP SPIFFS_CONFIG_SEP
#endif

Device::Device(const char* dname) {
    this->deviceName = NULL;
    this->setDeviceName(dname);
    this->hardwareAddress =
        (uint8_t*)malloc(HARDWARE_ADDRESS_LENGTH * sizeof(uint8_t));
    this->beginPacket = YOKIS_BEGIN_PACKET;
    this->endPacket = YOKIS_END_PACKET;
}

Device::Device(const Device* device) : Device(device->deviceName) {
    this->copy(device);
}

Device::Device(const char* dname, const uint8_t* hwAddr, uint8_t channel) : Device(dname) {
    this->setHardwareAddress(hwAddr);
    this->setChannel(channel);
}

Device::~Device() {
    free(this->deviceName);
    free(this->hardwareAddress);
}

const char* Device::getDeviceName() const { return this->deviceName; }

void Device::setDeviceName(const char* dname) {
    if (dname != NULL) {
        this->deviceName = (char*)realloc(this->deviceName, strlen(dname) + 1);
        strcpy(this->deviceName, dname);
    }
}

const uint8_t* Device::getHardwareAddress() const {
    return this->hardwareAddress;
}

uint8_t Device::getChannel() const { return this->channel; }

uint8_t Device::getBeginPacket() const { return this->beginPacket; }

uint8_t Device::getEndPacket() const { return this->endPacket; }

void Device::setBeginPacket(uint8_t p) { this->beginPacket = p; }

void Device::setEndPacket(uint8_t p) { this->endPacket = p; }

void Device::setHardwareAddress(const uint8_t* hwAddr) {
    if (hwAddr != NULL) {
        memcpy(this->hardwareAddress, hwAddr, HARDWARE_ADDRESS_LENGTH);
    }
}

void Device::setHardwareAddress(const char* hw) {
    uint8_t b1, b2;
    char buf[3];
    strncpy(buf, hw, 2);
    b1 = (uint8_t)strtoul(buf, NULL, 16);
    strncpy(buf, hw + 2, 2);
    b2 = (uint8_t)strtoul(buf, NULL, 16);
    this->hardwareAddress[0] = b1;
    this->hardwareAddress[1] = b2;
    this->hardwareAddress[2] = b1;
    this->hardwareAddress[3] = b2;
    this->hardwareAddress[4] = b2;
}

void Device::setChannel(uint8_t channel) { this->channel = channel; }

void Device::printDebugInfo() {
    Serial.println(deviceName);
    Serial.print("hw: ");
    Serial.print(hardwareAddress[0], HEX);
    Serial.print(" ");
    Serial.println(hardwareAddress[1], HEX);
    Serial.print("channel: ");
    Serial.println(channel, HEX);
    Serial.print("begin/end packets: ");
    Serial.print(beginPacket, HEX);
    Serial.print(" ");
    Serial.println(endPacket, HEX);
}

// Copy all fields from given device to this device
void Device::copy(const Device* d) {
    this->setDeviceName(d->getDeviceName());
    this->setHardwareAddress(d->getHardwareAddress());
    this->setChannel(d->getChannel());
    this->setBeginPacket(d->getBeginPacket());
    this->setEndPacket(d->getEndPacket());
}

// Static - Get device from a given list of devices
// devices is the list of devices
// size is the size of the list given
// deviceName is the name of the device to look for
// returns a pointer to the corresponding device if found, NULL otherwise
Device* Device::getFromList(Device** devices, size_t size,
                            const char* deviceName) {
    Device* d = NULL;

    for (unsigned int i = 0; i < size; i++) {
        if (devices[i] != NULL &&
            strcmp(devices[i]->getDeviceName(), deviceName) == 0) {
            d = devices[i];
            break;
        }
    }

    return d;
}

#ifdef ESP8266
bool Device::spiffsInitialized = false;

// Static - Initialize SPIFFS memory area
void Device::spiffsInit() {
    if (!Device::spiffsInitialized) {
        /*
        SPIFFSConfig cfg;
        cfg.setAutoFormat(false);
        SPIFFS.setConfig(cfg);
        */
        Device::spiffsInitialized = SPIFFS.begin();
    }
}

bool Device::saveToSpiffs() {
    bool ret = true;

    Device::spiffsInit();

    // Search for this device if it is already stored
    // delete the line if found
    int line = Device::findInConfig(deviceName);
    Device::deleteLineInConfig(line);

    File f = SPIFFS.open(SPIFFS_CONFIG_FILENAME, "a+");
    if (!f) {
        Serial.print(SPIFFS_CONFIG_FILENAME);
        Serial.println(" - file open failed");
        return false;
    }

    char buf[128];
    sprintf(buf, "%s%s%02x%02x%s%02x%s%02x%s%02x", deviceName, SEP,
            hardwareAddress[0], hardwareAddress[1], SEP, channel, SEP,
            beginPacket, SEP, endPacket);
    int bytesWritten = f.println(buf);
    if (bytesWritten <= 0) {
        Serial.print(SPIFFS_CONFIG_FILENAME);
        Serial.println(" - cannot write to file");
        ret = false;
    }

    f.close();
    return ret;
}

// Static - load devices previously stored in the SPIFFS memory area
void Device::loadFromSpiffs(Device** devices, const unsigned int size) {
    char buf[128];
    char* tok;
    uint16_t numLines = 0;
    Device* d = NULL;

    Device::spiffsInit();

    File f = SPIFFS.open(SPIFFS_CONFIG_FILENAME, "r");
    if (!f) {
        Serial.print(SPIFFS_CONFIG_FILENAME);
        Serial.println(" - File open failed");
        return;
    }

    while (f.available()) {
        if (numLines >= size) break;

        int l = f.readBytesUntil('\n', buf, sizeof(buf));
        buf[l] = 0;

        tok = strtok(buf, SEP);  // device name
        d = new Device(tok);

        tok = strtok(NULL,
                     SEP);  // hw address as two bytes represented as 4 chars
        d->setHardwareAddress(tok);

        tok = strtok(NULL, SEP);  // channel represented as 2 chars
        d->setChannel((uint8_t)strtoul(tok, NULL, 16));

        tok = strtok(NULL, SEP);  // begin packet represented as 2 chars
        d->setBeginPacket((uint8_t)strtoul(tok, NULL, 16));

        tok = strtok(NULL, SEP);  // end packet represented as 2 chars
        d->setEndPacket((uint8_t)strtoul(tok, NULL, 16));

        // Store this device
        devices[numLines++] = d;
        Serial.print("Added new device: ");
        Serial.println(d->getDeviceName());
    }

    f.close();
}

// static - display config file from SPIFFS
void Device::displayConfigFromSpiffs() {
    Device::spiffsInit();
    File f = SPIFFS.open(SPIFFS_CONFIG_FILENAME, "r");
    Serial.println("SPIFFS configuration stored:");
    while (f.available()) {
        Serial.write(f.read());
    }
    f.close();
}

// static
void Device::clearConfigFromSpiffs() {
    Device::spiffsInit();
    File f = SPIFFS.open(SPIFFS_CONFIG_FILENAME, "w");
    if (f) f.close();
}

// static
int Device::findInConfig(const char* deviceName) {
    Device::spiffsInit();
    int found = -1;
    int currentLine = 1;
    char buf[128];
    char* tok;
    File f = SPIFFS.open(SPIFFS_CONFIG_FILENAME, "r");
    if (!f) {
        Serial.println("File open failed");
        return found;
    }

    while (f.available()) {
        int l = f.readBytesUntil('\n', buf, sizeof(buf));
        buf[l] = 0;
        tok = strtok(buf, SEP);
        if (tok != NULL && strcmp(tok, deviceName) == 0) {
            found = currentLine;
            break;
        }

        currentLine++;
    }

    f.close();
    return found;
}

// static
void Device::deleteLineInConfig(int line) {
    Device::spiffsInit();

    char buf[128];
    int currentLine = 1;

    File f = SPIFFS.open(SPIFFS_CONFIG_FILENAME, "r");
    if (!f) {
        Serial.print(SPIFFS_CONFIG_FILENAME);
        Serial.println(" - File open failed");
        return;
    }
    File fbak = SPIFFS.open(SPIFFS_CONFIG_BAK_FILENAME, "w");
    if (!fbak) {
        Serial.print(SPIFFS_CONFIG_BAK_FILENAME);
        Serial.println(" - File open failed");
        return;
    }

    while (f.available()) {
        int l = f.readBytesUntil('\n', buf, sizeof(buf));
        buf[l] = 0;
        if (currentLine++ != line) {
            int bytesWritten = fbak.println(buf);
            if (bytesWritten <= 0) {
                Serial.println("Cannot write to backup configuration file");
            }
        }
    }

    f.close();
    fbak.close();
    SPIFFS.remove(SPIFFS_CONFIG_FILENAME);
    SPIFFS.rename(SPIFFS_CONFIG_BAK_FILENAME, SPIFFS_CONFIG_FILENAME);
}

#endif
