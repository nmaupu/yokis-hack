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
    this->beginPacket = YOKIS_DEFAULT_BEGIN_PACKET;
    this->endPacket = YOKIS_DEFAULT_END_PACKET;
    memset(this->serial, 0, 2 * sizeof(uint8_t));
    memset(this->version, 0, 3 * sizeof(uint8_t));
    this->setMode(ON_OFF);  // most used devices AFAIK
    this->setStatus(UNDEFINED);
    brightness = BRIGHTNESS_OFF;
}

Device::Device(const Device* device) : Device(device->deviceName) {
    this->copy(device);
}

Device::Device(const char* dname, const uint8_t* hwAddr, uint8_t channel)
    : Device(dname) {
    this->setHardwareAddress(hwAddr);
    this->setChannel(channel);
}

Device::Device(const char* dname, const uint8_t* hwAddr, uint8_t channel,
               const uint8_t* serial, const uint8_t* version)
    : Device(dname, hwAddr, channel) {
    this->setSerial(serial);
    this->setVersion(version);
}

Device::~Device() {
    free(this->deviceName);
    free(this->hardwareAddress);
}

const char* Device::getDeviceName() const { return this->deviceName; }

const uint8_t* Device::getHardwareAddress() const {
    return this->hardwareAddress;
}

uint8_t Device::getChannel() const { return this->channel; }

uint8_t Device::getBeginPacket() const { return this->beginPacket; }

uint8_t Device::getEndPacket() const { return this->endPacket; }

const uint8_t* Device::getVersion() const { return this->version; }

const uint8_t* Device::getSerial() const { return this->serial; }

const DeviceMode Device::getMode() const { return mode; }

const DeviceStatus Device::getStatus() const { return status; }

const DimmerBrightness Device::getBrightness() const { return brightness; }

// static
const char* Device::getStatusAsString(DeviceStatus status) {
    switch (status) {
        case ON:
            return "ON";
        case OFF:
            return "OFF";
        case UNDEFINED:
            return "UNDEFINED";
    }

    return NULL;
}

void Device::setDeviceName(const char* dname) {
    if (dname != NULL) {
        this->deviceName = (char*)realloc(this->deviceName, strlen(dname) + 1);
        strcpy(this->deviceName, dname);
    }
}

void Device::setBeginPacket(uint8_t p) { this->beginPacket = p; }

void Device::setEndPacket(uint8_t p) { this->endPacket = p; }

void Device::setHardwareAddress(const uint8_t* hwAddr) {
    if (hwAddr != NULL) {
        memcpy(this->hardwareAddress, hwAddr,
               HARDWARE_ADDRESS_LENGTH * sizeof(uint8_t));
    }
}

void Device::setHardwareAddress(const char* hw) {
    uint8_t b1, b2;
    char buf[3];

    buf[2] = 0;  // null terminate in advance

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

void Device::setVersion(const uint8_t* version) {
    memcpy(this->version, version, 3 * sizeof(uint8_t));
}

void Device::setSerial(const uint8_t* serial) {
    memcpy(this->serial, serial, 2 * sizeof(uint8_t));
}

void Device::setMode(DeviceMode mode) { this->mode = mode; }

void Device::setMode(const char* mode) {
    if (mode == NULL || strlen(mode) == 0) {
        this->setMode(ON_OFF);
    } else if (strcmp("DIMMER", mode) == 0) {
        this->setMode(DIMMER);
    } else if (strcmp("NO_RCPT", mode) == 0) {
        this->setMode(NO_RCPT);
    } else {
        this->setMode(ON_OFF);
    }
}

void Device::setStatus(DeviceStatus status) {
    this->status = status;
    if (this->status == OFF) this->setBrightness(BRIGHTNESS_OFF);
}

void Device::setBrightness(DimmerBrightness brightness) {
    this->brightness = brightness;
}

void Device::toggleStatus() {
    Serial.print("Current device status: ");
    Serial.println(Device::getStatusAsString(status));

    switch (status) {
        case UNDEFINED:
            setStatus(UNDEFINED);
            break;
        case OFF:
            setStatus(ON);
            break;
        case ON:
            setStatus(OFF);
            setBrightness(BRIGHTNESS_OFF);
            break;
    }

    Serial.print("New device status: ");
    Serial.println(Device::getStatusAsString(status));
}

void Device::toSerial() {
    Serial.print(deviceName);
    Serial.print(" - status=");
    Serial.println(Device::getStatusAsString(status));
    Serial.print("mode: ");
    Serial.println(mode);
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
    Serial.print("serial/version: ");
    Serial.print(serial[0], HEX);
    Serial.print(" ");
    Serial.print(serial[1], HEX);
    Serial.print("/");
    Serial.print(version[0], HEX);
    Serial.print(" ");
    Serial.print(version[1], HEX);
    Serial.print(" ");
    Serial.println(version[2], HEX);
}

// Copy all fields from given device to this device
void Device::copy(const Device* d) {
    this->setDeviceName(d->getDeviceName());
    this->setHardwareAddress(d->getHardwareAddress());
    this->setChannel(d->getChannel());
    this->setBeginPacket(d->getBeginPacket());
    this->setEndPacket(d->getEndPacket());
    this->setVersion(d->getVersion());
    this->setSerial(d->getSerial());
    this->setMode(d->getMode());
    this->setStatus(d->getStatus());
    this->setBrightness(d->getBrightness());
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
    sprintf(buf, "%s%s%02x%02x%s%02x%s%02x%s%02x%s%02x%02x%02x%s%02x%02x%s%04d",
            deviceName, SEP, hardwareAddress[0], hardwareAddress[1], SEP,
            channel, SEP, beginPacket, SEP, endPacket, SEP, version[0],
            version[1], version[2], SEP, serial[0], serial[1], SEP, mode);
    int bytesWritten = f.println(buf);
    if (bytesWritten <= 0) {
        Serial.print(SPIFFS_CONFIG_FILENAME);
        Serial.println(" - cannot write to file");
        ret = false;
    }

    f.close();
    return ret;
}

// Static - delete a device from SPIFFS configuration
void Device::deleteFromConfig(const char* deviceName) {
    int line = Device::findInConfig(deviceName);
    if (line != -1) Device::deleteLineInConfig(line);
}

// Static - load devices previously stored in the SPIFFS memory area
void Device::loadFromSpiffs(Device** devices, const unsigned int size) {
    char buf[128];
    char* tok;
    uint16_t numLines = 0;
    Device* d = NULL;
    char uCharBuf[3];
    uint8_t uIntBuf[3];

    Device::spiffsInit();

    File f = SPIFFS.open(SPIFFS_CONFIG_FILENAME, "r");
    if (!f) {
        Serial.print(SPIFFS_CONFIG_FILENAME);
        Serial.println(" - File open failed");
        return;
    }

    // null terminate in advance
    uCharBuf[2] = 0;

    while (f.available()) {
        if (numLines >= size) break;

        int l = f.readBytesUntil('\n', buf, sizeof(buf) - 1);
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

        tok = strtok(NULL, SEP);  // version represented as 6 chars
        if (tok != NULL) {
            strncpy(uCharBuf, tok, 2);
            uIntBuf[0] = (uint8_t)strtoul(uCharBuf, NULL, 16);
            strncpy(uCharBuf, tok + 2, 2);
            uIntBuf[1] = (uint8_t)strtoul(uCharBuf, NULL, 16);
            strncpy(uCharBuf, tok + 4, 2);
            uIntBuf[2] = (uint8_t)strtoul(uCharBuf, NULL, 16);

            d->setVersion(uIntBuf);
        }

        tok = strtok(NULL, SEP);  // Serial represented as 4 chars
        if (tok != NULL) {
            strncpy(uCharBuf, tok, 2);
            uIntBuf[0] = (uint8_t)strtoul(uCharBuf, NULL, 16);
            strncpy(uCharBuf, tok + 2, 2);
            uIntBuf[1] = (uint8_t)strtoul(uCharBuf, NULL, 16);

            d->setSerial(uIntBuf);
        }

        tok = strtok(NULL, SEP);  // Mode represented as 4 chars
        if (tok != NULL) {
            d->setMode((DeviceMode)strtoul(tok, NULL, 16));
        }

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
    if (deviceName == NULL) return -1;

    Device::spiffsInit();
    int currentLine = 1;
    char buf[128];
    char* tok;
    int found = -1;

    File f = SPIFFS.open(SPIFFS_CONFIG_FILENAME, "r");
    if (!f) {
        Serial.println("File open failed");
        return -1;
    }

    while (f.available()) {
        int l = f.readBytesUntil('\n', buf, sizeof(buf) - 1);
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
        int l = f.readBytesUntil('\n', buf, sizeof(buf) - 1);
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
