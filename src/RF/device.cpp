#include "RF/device.h"

#include "globals.h"
#include "utils.h"

#if defined(ESP8266)
#include <FS.h>
#endif

Device::Device(const char* dname) {
    this->name = NULL;
    this->setName(dname);
    this->hardwareAddress =
        (uint8_t*)malloc(HARDWARE_ADDRESS_LENGTH * sizeof(uint8_t));
    memset(this->serial, 0, 2 * sizeof(uint8_t));
    memset(this->version, 0, 3 * sizeof(uint8_t));
    this->setMode(ON_OFF);  // most used devices AFAIK
    this->setStatus(UNDEFINED);
    this->setAvailability(ONLINE);
    this->setBrightness(BRIGHTNESS_OFF);
    this->lastUpdateMillis = 0;
    this->hasToBePolledForStatus = false;
}

Device::Device(const Device* device) : Device(device->name) {
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
    free(this->name);
    free(this->hardwareAddress);
}

const char* Device::getName() const { return this->name; }

const uint8_t* Device::getHardwareAddress() const {
    return this->hardwareAddress;
}

uint8_t Device::getChannel() const { return this->channel; }

const uint8_t* Device::getVersion() const { return this->version; }

const uint8_t* Device::getSerial() const { return this->serial; }

const DeviceMode Device::getMode() const { return mode; }

const DeviceStatus Device::getStatus() const { return status; }

const DimmerBrightness Device::getBrightness() const { return brightness; }

const DeviceAvailability Device::getAvailability() const {
    return availability;
}

const unsigned long Device::getLastUpdateMillis() const {
    return lastUpdateMillis;
}

bool Device::needsPolling() { return hasToBePolledForStatus; }

// static
const char* Device::getStatusAsString(DeviceStatus status) {
    switch (status) {
        case ON:
            return "ON";
        case OFF:
            return "OFF";
        case UNDEFINED:
            return "UNDEFINED";
        case PAUSE_SHUTTER:
            return "PAUSE_SHUTTER";
    }

    return NULL;
}

// static
const char* Device::getModeAsString(DeviceMode mode) {
    switch (mode) {
        case DIMMER:
            return "DIMMER";
        case ON_OFF:
            return "ON_OFF";
        case SHUTTER:
           return "SHUTTER";
        case SHUTTER_BUS:
           return "SHUTTER_BUS";
        case NO_RCPT:
            return "NO_RCP";
    }

    return NULL;
}

// static
const char* Device::getAvailabilityAsString(DeviceAvailability availability) {
    switch (availability) {
        case ONLINE:
            return "Online";
        case OFFLINE:
            return "Offline";
    }

    return NULL;
}

void Device::setName(const char* dname) {
    if (dname != NULL) {
        this->name = (char*)realloc(this->name, strlen(dname) + 1);
        strcpy(this->name, dname);
    }
}

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
    } else if (strcmp("SHUTTER", mode) == 0) {
        this->setMode(SHUTTER);
    } else if (strcmp("SHUTTER_BUS", mode) ==0) {
        this->setMode(SHUTTER_BUS);
    } else if (strcmp("NO_RCPT", mode) == 0) {
        this->setMode(NO_RCPT);
    } else {
        this->setMode(ON_OFF);
    }
}

void Device::setStatus(DeviceStatus status) {
    this->status = status;
    if (this->status == OFF) this->setBrightness(BRIGHTNESS_OFF);
    this->lastUpdateMillis = millis();
}

void Device::setBrightness(DimmerBrightness brightness) {
    this->brightness = brightness;
    this->lastUpdateMillis = millis();
}

void Device::setAvailability(DeviceAvailability availability) {
    this->availability = availability;
}

void Device::online() { this->setAvailability(ONLINE); }
void Device::offline() {
    this->setAvailability(OFFLINE);
    this->setStatus(UNDEFINED);
}
bool Device::isOnline() { return this->availability == ONLINE; }
bool Device::isOffline() { return this->availability == OFFLINE; }

void Device::pollMePlease() { this->hasToBePolledForStatus = true; }

void Device::pollingSuccess() {
    this->hasToBePolledForStatus = false;
    this->failedPolls = 0;
}

uint8_t Device::pollingFailed() {
    this->hasToBePolledForStatus = false;
    return ++failedPolls;
}

uint8_t Device::getFailedPollings() {
    return this->failedPolls;
}

void Device::toggleStatus() {
    switch (status) {
        case UNDEFINED:
            setStatus(UNDEFINED);
            break;
        case PAUSE_SHUTTER:
            setStatus(PAUSE_SHUTTER);
            break;
        case OFF:
            setStatus(ON);
            break;
        case ON:
            setStatus(OFF);
            setBrightness(BRIGHTNESS_OFF);
            break;
    }
}

void Device::toSerial() {
    LOG.print(name);
    LOG.print(" - status=");
    LOG.println(Device::getStatusAsString(status));
    LOG.print("Availability: ");
    LOG.println(Device::getAvailabilityAsString(availability));
    LOG.print("mode: ");
    LOG.println(Device::getModeAsString(mode));
    LOG.print("hw: ");
    LOG.print(hardwareAddress[0], HEX);
    LOG.print(" ");
    LOG.println(hardwareAddress[1], HEX);
    LOG.print("channel: ");
    LOG.println(channel, HEX);
    LOG.print("serial/version: ");
    LOG.print(serial[0], HEX);
    LOG.print(" ");
    LOG.print(serial[1], HEX);
    LOG.print("/");
    LOG.print(version[0], HEX);
    LOG.print(" ");
    LOG.print(version[1], HEX);
    LOG.print(" ");
    LOG.println(version[2], HEX);
}

// Copy all fields from given device to this device
void Device::copy(const Device* d) {
    this->setName(d->getName());
    this->setHardwareAddress(d->getHardwareAddress());
    this->setChannel(d->getChannel());
    this->setVersion(d->getVersion());
    this->setSerial(d->getSerial());
    this->setMode(d->getMode());
    this->setStatus(d->getStatus());
    this->setBrightness(d->getBrightness());
    this->setAvailability(d->getAvailability());
    this->lastUpdateMillis = d->lastUpdateMillis;
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
            strcmp(devices[i]->getName(), deviceName) == 0) {
            d = devices[i];
            break;
        }
    }

    return d;
}

#ifdef ESP8266

// Static - Store a raw configuration line into LittleFS
bool Device::storeRawConfig(const char* line) {
    bool ret = true;

    YokisLittleFS::init();

    File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "a+");
    if (!f) {
        LOG.print(LITTLEFS_CONFIG_FILENAME);
        LOG.println(" - file open failed");
        return false;
    }

    char buf[128];
    sprintf(buf, "%s", line);
    int bytesWritten = f.println(buf);
    if (bytesWritten <= 0) {
        LOG.print(LITTLEFS_CONFIG_FILENAME);
        LOG.println(" - cannot write to file");
        ret = false;
    }

    f.close();
    return ret;
}

bool Device::saveToLittleFS() {
    bool ret = true;

    YokisLittleFS::init();

    // Search for this device if it is already stored
    // delete the line if found
    int line = Device::findInConfig(name);
    Device::deleteLineInConfig(line);

    File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "a+");
    if (!f) {
        LOG.print(LITTLEFS_CONFIG_FILENAME);
        LOG.println(" - file open failed");
        return false;
    }

    char buf[128];
    sprintf(buf, "%s%s%02x%02x%s%02x%s%02x%s%02x%s%02x%02x%02x%s%02x%02x%s%04d",
            name, SEP, hardwareAddress[0], hardwareAddress[1], SEP, channel,
            SEP, 0, SEP, 0, SEP, version[0], version[1],
            version[2], SEP, serial[0], serial[1], SEP, mode);
    int bytesWritten = f.println(buf);
    if (bytesWritten <= 0) {
        LOG.print(LITTLEFS_CONFIG_FILENAME);
        LOG.println(" - cannot write to file");
        ret = false;
    }

    f.close();
    return ret;
}

// Static - delete a device from LittleFS configuration
void Device::deleteFromConfig(const char* deviceName) {
    int line = Device::findInConfig(deviceName);
    if (line != -1) Device::deleteLineInConfig(line);
}

// Static - load devices previously stored in the LittleFS memory area
void Device::loadFromLittleFS(Device** devices, const unsigned int size) {
    char buf[128];
    char* tok;
    uint16_t numLines = 0;
    Device* d = NULL;
    char uCharBuf[3];
    uint8_t uIntBuf[3];

    YokisLittleFS::init();

    File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "r");
    if (!f) {
        LOG.print(LITTLEFS_CONFIG_FILENAME);
        LOG.println(" - File open failed");
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

        tok = strtok(NULL, SEP);  // begin packet represented as 2 chars - not used anymore
        //d->setBeginPacket((uint8_t)strtoul(tok, NULL, 16));

        tok = strtok(NULL, SEP);  // end packet represented as 2 chars - not used anymore
        //d->setEndPacket((uint8_t)strtoul(tok, NULL, 16));

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
        LOG.print("Added new device: ");
        LOG.println(d->getName());
    }

    f.close();
}

// static - display config file from LittleFS
void Device::displayConfigFromLittleFS() {
    YokisLittleFS::init();
    File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "r");
    LOG.println("LittleFS configuration stored:");
    while (f.available()) {
        LOG.write(f.read());
    }
    f.close();
}

// static
void Device::clearConfigFromLittleFS() {
    YokisLittleFS::init();
    File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "w");
    if (f) f.close();
}

// static
int Device::findInConfig(const char* deviceName) {
    if (deviceName == NULL) return -1;

    YokisLittleFS::init();
    int currentLine = 1;
    char buf[128];
    char* tok;
    int found = -1;

    File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "r");
    if (!f) {
        LOG.println("File open failed");
        return -1;
    }

    while (f.available()) {
        int l = f.readBytesUntil('\n', buf, sizeof(buf) - 1);
        buf[l] = 0; // terminate string with a null char
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
    if(line <= 0) {
        return;
    }

    YokisLittleFS::init();

    char buf[128];
    int currentLine = 1;

    File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "r");
    if (!f) {
        LOG.print(LITTLEFS_CONFIG_FILENAME);
        LOG.println(" - File open failed");
        return;
    }
    File fbak = LittleFS.open(LITTLEFS_CONFIG_BAK_FILENAME, "w");
    if (!fbak) {
        LOG.print(LITTLEFS_CONFIG_BAK_FILENAME);
        LOG.println(" - File open failed");
        return;
    }

    while (f.available()) {
        int l = f.readBytesUntil('\n', buf, sizeof(buf) - 1);
        buf[l] = 0;
        if (currentLine++ != line) {
            int bytesWritten = fbak.println(buf);
            if (bytesWritten <= 0) {
                LOG.println("Cannot write to backup configuration file");
            }
        }
    }

    f.close();
    fbak.close();
    LittleFS.remove(LITTLEFS_CONFIG_FILENAME);
    LittleFS.rename(LITTLEFS_CONFIG_BAK_FILENAME, LITTLEFS_CONFIG_FILENAME);
}

#endif
