#include <Arduino.h>
#include "RF/copy.h"
#include "RF/e2bp.h"
#include "RF/irqManager.h"
#include "RF/pairing.h"
#include "RF/scanner.h"
#include "globals.h"
#include "printf.h"
#include "serial/genericCallback.h"
#include "serial/serialHelper.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include "net/mqttHass.h"
#include "net/wifi.h"
#endif

// globals' initialization
byte g_ConfigFlags = ~FLAG_RAW & ~FLAG_DEBUG & FLAG_POLLING;
SerialHelper* g_serial;
Pairing* g_pairingRF;
E2bp* g_bp;
Scanner* g_scanner;
Copy* g_copy;
#ifdef ESP8266
MqttHass* g_mqtt;
// no need to store more devices than supported by MQTT
Device* devices[MQTT_MAX_NUM_OF_YOKIS_DEVICES];
#endif
IrqType IrqManager::irqType = PAIRING;
Device* currentDevice;

#define CURRENT_DEVICE_DEFAULT_NAME "tempDevice"

// MQTT configuration via compile options for ESP8266
#ifdef ESP8266
// status led to turn off
#define STATUS_LED D4

Ticker* deviceStatusPollers[MQTT_MAX_NUM_OF_YOKIS_DEVICES];

// Don't update the same device during the MQTT_UPDATE_MILLIS_WINDOW
// time window !
// HASS sends sometimes multiple times the same MQTT message in a row...
// Usually 2 MQTT messages sent in a row are processed very quickly
// something like 3 to 5 ms
#define MQTT_UPDATE_MILLIS_WINDOW 100

WiFiClient espClient;
#ifdef MQTT_IP
const char host[] = MQTT_IP;
#else
const char host[] = "192.168.0.1";
#endif

#ifdef MQTT_PORT
uint16_t port = (uint16_t)atoi(MQTT_PORT);
#else
uint16_t port = 1883;
#endif

#ifdef MQTT_USERNAME
char mqttUser[] = MQTT_USERNAME;
#else
char mqttUser[] = "mqtt";
#endif

#ifdef MQTT_PASSWORD
char mqttPassword[] = MQTT_PASSWORD;
#else
char mqttPassword[] = "password";
#endif

bool mqttInit = false;

#endif  // ESP8266

// Callback functions
bool pairingCallback(const char*);
bool onCallback(const char*);
bool offCallback(const char*);
bool toggleCallback(const char*);
bool scannerCallback(const char*);
bool copyCallback(const char*);
bool displayDevices(const char*);
bool pressCallback(const char*);
bool pressForCallback(const char*);
bool releaseCallback(const char*);
bool statusCallback(const char*);
bool dimmerMemCallback(const char*);
bool dimmerMaxCallback(const char*);
bool dimmerMidCallback(const char*);
bool dimmerMinCallback(const char*);
bool dimmerNilCallback(const char*);
bool dimmerSet(const char*, const uint8_t);

#ifdef ESP8266
bool storeConfigCallback(const char*);
bool clearConfig(const char*);
bool displayConfig(const char*);
bool reloadConfig(const char*);
bool deleteFromConfig(const char*);
Device* getDeviceFromParams(const char*);
void pollDevice(Device* device);  // Interrupt func
void pollForStatus(Device* device);
void mqttCallback(char*, uint8_t*, unsigned int);
#endif

void setup() {
    randomSeed(micros());

    // Globals' initialization
    g_serial = new SerialHelper();
    g_pairingRF = new Pairing(CE_PIN, CSN_PIN);
    g_bp = new E2bp(CE_PIN, CSN_PIN);
    g_scanner = new Scanner(CE_PIN, CSN_PIN);
    g_copy = new Copy(CE_PIN, CSN_PIN);
    currentDevice = new Device(CURRENT_DEVICE_DEFAULT_NAME);

#ifdef ESP8266
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, HIGH);  // pin is inverted so, set it off

    // Load all previously stored devices from SPIFFS memory
    reloadConfig(NULL);

    setupWifi();
    g_mqtt = new MqttHass(espClient, host, &port, mqttUser, mqttPassword);
    g_mqtt->setCallback(mqttCallback);
#endif

    // Serial setup
    g_serial->registerCallback(new GenericCallback(
        "pair",
        "Pair with a Yokis device - basically act as "
        "if a Yokis remote is in pairing mode (5 button clicks)",
        pairingCallback));
    g_serial->registerCallback(
        new GenericCallback("toggle",
                            "send a toggle message - basically act as a Yokis "
                            "remote when a button is pressed then released",
                            toggleCallback));
    g_serial->registerCallback(
        new GenericCallback("scan",
                            "Scan the network for packets - polling has to be "
                            "disabled for this to work",
                            scannerCallback));
    g_serial->registerCallback(new GenericCallback(
        "copy",
        "Copy a device to a pairing one (or disconnect if already configured)",
        copyCallback));
    g_serial->registerCallback(new GenericCallback(
        "dConfig", "display loaded config / current config", displayDevices));
    g_serial->registerCallback(new GenericCallback(
        "on", "Switch ON the configured device", onCallback));
    g_serial->registerCallback(new GenericCallback(
        "off", "Switch OFF the configured device", offCallback));
    g_serial->registerCallback(new GenericCallback(
        "press", "Press and hold an e2bp button", pressCallback));
    g_serial->registerCallback(new GenericCallback(
        "pressFor", "Press and hold for x milliseconds", pressForCallback));
    g_serial->registerCallback(new GenericCallback(
        "release", "Release an e2bp button", releaseCallback));
    g_serial->registerCallback(
        new GenericCallback("status", "Get device status", statusCallback));
    g_serial->registerCallback(new GenericCallback(
        "dimmem", "Set a dimmer to memory (= 1 button pushes)",
        dimmerMemCallback));
    g_serial->registerCallback(new GenericCallback(
        "dimmax", "Set a dimmer to maximum (= 2 button pushes)",
        dimmerMaxCallback));
    g_serial->registerCallback(new GenericCallback(
        "dimmid", "Set a dimmer to middle (= 3 button pushes)",
        dimmerMidCallback));
    g_serial->registerCallback(new GenericCallback(
        "dimmin", "Set a dimmer to minimum (= 4 button pushes)",
        dimmerMinCallback));
    g_serial->registerCallback(new GenericCallback(
        "dimnil", "Set a dimmer to night light mode (= 7 button pushes)",
        dimmerNilCallback));

#ifdef ESP8266
    g_serial->registerCallback(new GenericCallback(
        "save", "Save current device configuration to SPIFFS",
        storeConfigCallback));
    g_serial->registerCallback(new GenericCallback(
        "delete", "Delete one entry fron SPIFFS configuration",
        deleteFromConfig));
    g_serial->registerCallback(new GenericCallback(
        "clear", "Clear all config previously stored to SPIFFS", clearConfig));
    g_serial->registerCallback(new GenericCallback(
        "reload", "Reload config from SPIFFS to memory", reloadConfig));
    g_serial->registerCallback(new GenericCallback(
        "dSpiffs", "display config previously stored in SPIFFS",
        displayConfig));
#endif

    // Handle interrupt pin
    pinMode(IRQ_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(IRQ_PIN), IrqManager::processIRQ,
                    FALLING);

    printf_begin();  // Works only for Arduino devices...
    g_serial->executeCallback("help");
    Serial.println();
    g_serial->prompt();
}

void loop() {
#if defined(ESP8266)
    g_mqtt->loop();

    uint8_t nbDevices = 0;
    if (!mqttInit) {
        Serial.print("Publishing homeassistant discovery data... ");
        for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
            if (devices[i] != NULL) {
                nbDevices++;
                if (g_mqtt->publishDevice(devices[i])) {
                    g_mqtt->subscribeDevice(devices[i]);
                    mqttInit = true;
                } else {
                    Serial.println("KO");
                    break;
                }
            }
        }

        if (nbDevices == 0)
            mqttInit = true;
        if (mqttInit)
            Serial.println("OK");
    } else {
        // Verify polling statuses and update via MQTT if needed
        for (uint8_t i = 0;
             i < MQTT_MAX_NUM_OF_YOKIS_DEVICES && FLAG_IS_ENABLED(FLAG_POLLING);
             i++) {
            if (devices[i] != NULL && devices[i]->needsPolling()) {
                pollForStatus(devices[i]);
            }
        }
    }
#endif
    g_serial->readFromSerial();
    delay(1);
}

bool pairingCallback(const char*) {
    uint8_t buf[5];  // enough size for addr, serial and version
    IrqManager::irqType = PAIRING;
    bool res = g_pairingRF->hackPairing();
    if (res) {
        g_pairingRF->getAddressFromRecvData(buf);
        currentDevice->setHardwareAddress(buf);
        currentDevice->setChannel(g_pairingRF->getChannelFromRecvData());

        g_pairingRF->getSerialFromRecvData(buf);
        currentDevice->setSerial(buf);

        g_pairingRF->getVersionFromRecvData(buf);
        currentDevice->setVersion(buf);

        // Get device status and get device mode
        statusCallback(NULL);
        currentDevice->setMode(g_bp->getDeviceModeFromRecvData());

        currentDevice->toSerial();
    }
    return res;
}

// Get a device from the list with the given params
Device* getDeviceFromParams(const char* params) {
#ifdef ESP8266
    if (params == NULL || strcmp("", params) == 0) return currentDevice;

    char* paramsBak;
    char* pch;
    Device* d;

    int len = strlen(params);
    paramsBak = new char[len + 1];
    strncpy(paramsBak, params, len);
    paramsBak[len] = 0;
    strtok(paramsBak, " ");   // ignore the command name
    pch = strtok(NULL, " ");  // get the name of the device

    if (pch == NULL || strcmp("", pch) == 0) {
        d = currentDevice;
    } else {
        d = Device::getFromList(devices, MQTT_MAX_NUM_OF_YOKIS_DEVICES, pch);
    }

    delete[] paramsBak;
    return d;
#else
    return currentDevice;
#endif
}

// Generic on, off or toggle
bool changeDeviceState(const char* params, bool (E2bp::*func)(void)) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    bool ret = (g_bp->*func)();
#ifdef ESP8266
    if (ret) {
        if (d->getMode() == DIMMER) {
            g_mqtt->notifyBrightness(d);
        } else {
            g_mqtt->notifyPower(d);
        }
    }
#endif
    return ret;
}

bool toggleCallback(const char* params) {
    return changeDeviceState(params, &E2bp::toggle);
}

bool onCallback(const char* params) {
    return changeDeviceState(params, &E2bp::on);
}

bool offCallback(const char* params) {
    return changeDeviceState(params, &E2bp::off);
}

bool scannerCallback(const char* params) {
    if (FLAG_IS_ENABLED(FLAG_POLLING)) {
        Serial.println("Disable polling before attempting to scan ! Aborting.");
        return false;
    }

    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    IrqManager::irqType = SCANNER;
    g_scanner->setDevice(d);
    g_scanner->setupRFModule();

    return true;
}

bool copyCallback(const char* params) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    IrqManager::irqType = COPYING;
    g_copy->setDevice(d);
    return g_copy->send();
}

bool displayDevices(const char*) {
#ifdef ESP8266
    uint8_t c = 0;
    while (devices[c] != NULL) {
        Serial.println("=== Device ===");
        devices[c]->toSerial();
        Serial.println("==============");
        c++;
    }
#else
    // for arduino devices, only display the currentDevice
    currentDevice->toSerial();
#endif
    return true;
}

bool dimmerMemCallback(const char* params) {
    int ret = changeDeviceState(params, &E2bp::dimmerMem);
    return ret != -1;
}
bool dimmerMaxCallback(const char* params) {
    int ret = changeDeviceState(params, &E2bp::dimmerMax);
    return ret != -1;
}
bool dimmerMidCallback(const char* params) {
    int ret = changeDeviceState(params, &E2bp::dimmerMid);
    return ret != -1;
}
bool dimmerMinCallback(const char* params) {
    int ret = changeDeviceState(params, &E2bp::dimmerMin);
    return ret != -1;
}
bool dimmerNilCallback(const char* params) {
    int ret = changeDeviceState(params, &E2bp::dimmerNiL);
    return ret != -1;
}

bool pressCallback(const char* params) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    g_bp->reset();
    g_bp->setupRFModule();
    bool ret = g_bp->press();
    return ret;
}

bool pressForCallback(const char* params) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    char* tok;
    size_t paramsLen = strlen(params);
    char* paramsBak = new char[paramsLen + 1];
    strncpy(paramsBak, params, paramsLen);
    paramsBak[paramsLen] = 0;
    strtok(paramsBak, " ");   // command
    strtok(NULL, " ");        // device name
    tok = strtok(NULL, " ");  // duration
    unsigned long durationMs = strtoul(tok, NULL, 10);
    delete[] paramsBak;

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    return g_bp->pressAndHoldFor(durationMs);
}

bool releaseCallback(const char* params) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    g_bp->reset();
    g_bp->setupRFModule();
    bool ret = g_bp->release();
    return ret;
}

bool statusCallback(const char* params) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    DeviceStatus st = g_bp->pollForStatus();

    Serial.print("Device Status = ");
    Serial.println(Device::getStatusAsString(st));

    return true;
}

#ifdef ESP8266
bool storeConfigCallback(const char* params) {
    char* paramsBak;
    char* pch;
    bool ret;

    int len = strlen(params);
    paramsBak = new char[len + 1];
    strncpy(paramsBak, params, len);
    paramsBak[len] = 0;
    strtok(paramsBak, " ");   // ignore the command name
    pch = strtok(NULL, " ");  // get the name of the device
    currentDevice->setName(pch);

    // This should be auto detected from pairing
    pch = strtok(NULL, " ");  // Get the device mode
    if (pch != NULL) {
        currentDevice->setMode(pch);
    }

    ret = currentDevice->saveToSpiffs();
    if (ret) Serial.println("Saved.");

    // reset default name
    currentDevice->setName(CURRENT_DEVICE_DEFAULT_NAME);

    delete[] paramsBak;
    return ret;
}

bool clearConfig(const char*) {
    Device::clearConfigFromSpiffs();
    return true;
}

bool displayConfig(const char*) {
    Device::displayConfigFromSpiffs();
    return true;
}

bool reloadConfig(const char*) {
    for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
        delete devices[i];  // delete previously allocated device if needed
        if (deviceStatusPollers[i] != NULL) deviceStatusPollers[i]->detach();
        delete deviceStatusPollers[i];
        devices[i] = NULL;
        deviceStatusPollers[i] = NULL;
    }
    Device::loadFromSpiffs(devices, MQTT_MAX_NUM_OF_YOKIS_DEVICES);

    // Reattach tickers to devices
    for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
        if (devices[i] != NULL) {
            deviceStatusPollers[i] = new Ticker();
            deviceStatusPollers[i]->attach_ms(random(4000, 10000), pollDevice,
                                              devices[i]);
        }
    }
    Serial.println("Reloaded.");
    return true;
}

bool deleteFromConfig(const char* params) {
    char* paramsBak;
    char* pch;

    int len = strlen(params);
    paramsBak = new char[len + 1];
    strncpy(paramsBak, params, len);
    paramsBak[len] = 0;
    strtok(paramsBak, " ");   // ignore the command
    pch = strtok(NULL, " ");  // Get the name to delete

    Device::deleteFromConfig(pch);

    delete[] paramsBak;
    return true;
}

// Interrupt function
void pollDevice(Device* d) {
    if (FLAG_IS_ENABLED(FLAG_POLLING)) d->pollMePlease();
}

void pollForStatus(Device* d) {
    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    DeviceStatus ds = g_bp->pollForStatus();

    if (ds != UNDEFINED) {     // device reachable
        if(d->getFailedPollings() > 0) {
            Serial.print("Device ");
            Serial.print(d->getName());
            Serial.println(" recovered");
        }

        d->pollingSuccess();

        if (d->isOffline()) {  // Device is back online
            d->online();
            g_mqtt->notifyOnline(d);
        }

        // Update device status if changing
        if (d->getStatus() != ds) {
            d->setStatus(ds);
            if (d->getMode() == DIMMER) {
                if (ds == ON && d->getBrightness() == 0)
                    d->setBrightness(BRIGHTNESS_MAX);
                g_mqtt->notifyBrightness(d);
            } else {
                g_mqtt->notifyPower(d);
            }
        }
    } else {
        if (d->pollingFailed() >= DEVICE_MAX_FAILED_POLLING_BEFORE_OFFLINE){
            // Device is unreachable
            Serial.print("Device ");
            Serial.print(d->getName());
            Serial.println(" is offline");
            d->offline();
            g_mqtt->notifyOffline(d);
        } else {
            Serial.print("Failed to check device ");
            Serial.print(d->getName());
            Serial.print(" ");
            Serial.print(d->getFailedPollings(), DEC);
            Serial.print("/");
            Serial.println(DEVICE_MAX_FAILED_POLLING_BEFORE_OFFLINE, DEC);
        }
    }
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    char* tok;
    char* mTokBuf = NULL;
    char* mTopic = NULL;
    char* mCmnd = NULL;
    char* mPayload = NULL;
    Device* d;
    size_t len;

    // Topic copy
    len = strlen(topic);
    mTopic = new char[len + 1];
    strncpy(mTopic, topic, len);
    mTopic[len] = 0;

    // Get device
    mTokBuf = new char[len + 1];
    strncpy(mTokBuf, mTopic, len);
    mTokBuf[len] = 0;
    tok = strtok(mTokBuf, "/");  // device name
    d = Device::getFromList(devices, MQTT_MAX_NUM_OF_YOKIS_DEVICES, tok);

    // If we update this device too soon, ignore the payload
    unsigned long now = millis();
    if (d->getLastUpdateMillis() + MQTT_UPDATE_MILLIS_WINDOW > now) {
        Serial.println(
            "Ignoring MQTT message: received too soon for this device");
        Serial.print("Last update: ");
        Serial.println(d->getLastUpdateMillis(), DEC);
        Serial.print("This update: ");
        Serial.println(now, DEC);
        Serial.print("Difference: ");
        Serial.println(now - d->getLastUpdateMillis());

        delete[] mTopic;
        delete[] mTokBuf;
        return;
    }

    // Get cmnd type (POWER or BRIGHTNESS)
    tok = strtok(NULL, "/");  // cmnd
    tok = strtok(NULL, "/");  // POWER or BRIGHTNESS
    len = strlen(tok);
    mCmnd = new char[len + 1];
    strncpy(mCmnd, tok, len);
    mCmnd[len] = 0;

    // Get payload
    mPayload = new char[length + 1];
    strncpy(mPayload, (char*)payload, length);  // consider payload as char*
    mPayload[length] = 0;

    // Processing MQTT message
    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    switch (d->getMode()) {
        case ON_OFF:
        case NO_RCPT:
            if (strcmp(mPayload, "ON") == 0) {
                g_bp->on();
            } else if (strcmp(mPayload, "OFF") == 0) {
                g_bp->off();
            }

            g_mqtt->notifyPower(d);
            break;
        case DIMMER:
            // brightness will be 0 for anything that is not a number
            // so will set light to OFF for all possible POWER cases (ON OR
            // OFF) HASS will send only POWER OFF, never POWER ON because
            // on_command_type=brightness set on MQTT configuration (see
            // MqttHass class)
            int brightness = (uint8_t)atoi(mPayload);

            switch (brightness) {
                case BRIGHTNESS_OFF:
                    g_bp->off();
                    break;
                case BRIGHTNESS_MIN:
                    g_bp->dimmerMin();
                    break;
                case BRIGHTNESS_MID:
                    g_bp->dimmerMid();
                    break;
                default:  // MAX values
                    g_bp->on();
                    break;
            }

            g_mqtt->notifyBrightness(d);
            break;
    }

    delete[] mTopic;
    delete[] mTokBuf;
    delete[] mCmnd;
    delete[] mPayload;
}
#endif
