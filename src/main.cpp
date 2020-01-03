#include <Arduino.h>
#include "RF/e2bp.h"
#include "RF/irqManager.h"
#include "RF/pairing.h"
#include "globals.h"
#include "printf.h"
#include "serial/genericCallback.h"
#include "serial/serialHelper.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "net/mqttHass.h"
#include "net/wifi.h"
#endif

// globals' initialization
byte g_ConfigFlags = 0;
SerialHelper* g_serial;
Pairing* g_pairingRF;
E2bp* g_bp;
Scanner* g_scanner;
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
#endif

bool pairingCallback(const char*);
bool onCallback(const char*);
bool offCallback(const char*);
bool toggleCallback(const char*);
bool scannerCallback(const char*);
#ifdef ESP8266
bool storeConfigCallback(const char*);
bool clearConfig(const char*);
bool displayConfig(const char*);
bool displayDevices(const char*);
bool reloadConfig(const char*);
Device* getDeviceFromParams(const char*);
void mqttCallback(char*, uint8_t*, unsigned int);
#endif

    void setup() {
    randomSeed(micros());

    // Globals' initialization
    g_serial = new SerialHelper();
    g_pairingRF = new Pairing(CE_PIN, CSN_PIN);
    g_bp = new E2bp(CE_PIN, CSN_PIN);
    g_scanner = new Scanner(CE_PIN, CSN_PIN);
    currentDevice = new Device(CURRENT_DEVICE_DEFAULT_NAME);

#ifdef ESP8266
    // Load all previously stored devices from SPIFFS memory
    reloadConfig(NULL);

    setupWifi();
    g_mqtt = new MqttHass(espClient, host, &port, mqttUser, mqttPassword);
    g_mqtt->setCallback(mqttCallback);
#endif

        // Serial setup
        g_serial->registerCallback(
            new GenericCallback("pair",
                                "Pair with a Yokis device - basically act as "
                                "if a Yokis remote try is pairing",
                                pairingCallback));
    g_serial->registerCallback(
        new GenericCallback("toggle",
                            "send a toggle message - basically act as a Yokis "
                            "remote when button pressed",
                            toggleCallback));
    g_serial->registerCallback(
        new GenericCallback("on", "On the configured device", onCallback));
    g_serial->registerCallback(
        new GenericCallback("off", "On the configured device", offCallback));
    g_serial->registerCallback(new GenericCallback(
        "scan", "Scan the network for packets", scannerCallback));
#ifdef ESP8266
    g_serial->registerCallback(new GenericCallback(
        "save", "Save current device configuration to SPIFFS",
        storeConfigCallback));
    g_serial->registerCallback(new GenericCallback(
        "clear", "Clear all config previously stored to SPIFFS", clearConfig));
    g_serial->registerCallback(new GenericCallback(
        "reload", "Reload config from SPIFFS to memory", reloadConfig));
    g_serial->registerCallback(new GenericCallback(
        "dSpiffs", "display config previously stored in SPIFFS", displayConfig));
    g_serial->registerCallback(new GenericCallback(
        "dConfig", "display loaded config", displayDevices));
#endif

    // Handle interrupt pin
    pinMode(IRQ_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(IRQ_PIN), IrqManager::processIRQ,
                    FALLING);

    printf_begin();
    g_serial->executeCallback("help");
    Serial.println();
    g_serial->prompt();
}

bool initMqtt = false;
void loop() {
#if defined(ESP8266)
    g_mqtt->loop();

    if(!initMqtt) {
        Serial.print("Publishing homeassistant discovery data... ");
        for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
            if (devices[i] != NULL) {
                if (g_mqtt->publishDevice(devices[i])) {
                    g_mqtt->subscribeDevice(devices[i]);
                    initMqtt = true;
                } else {
                    Serial.println("KO");
                    break;
                }
            }
        }

        if (initMqtt) Serial.println("OK");
    }
#endif
    g_serial->readFromSerial();
    delay(1);
}

bool pairingCallback(const char*) {
    uint8_t addr[5];
    IrqManager::irqType = PAIRING;
    bool res = g_pairingRF->hackPairing();
    if (res) {
        g_pairingRF->getAddressFromRecvData(addr);
        currentDevice->setHardwareAddress(addr);
        currentDevice->setChannel(g_pairingRF->getChannelFromRecvData());
        currentDevice->printDebugInfo();
    }
    return res;
}

// Get a device from the list with the given params
Device* getDeviceFromParams(const char* params) {
#ifdef ESP8266
    char paramBak[128];
    char* pch;
    Device* d;

    strncpy(paramBak, params, 127);
    pch = GenericCallback::getNextParam(paramBak);  // get the command name
    pch = GenericCallback::getNextParam(NULL);  // Get the name of the device

    if (pch == NULL || strcmp(pch, "") == 0) {
        d = currentDevice;
    } else {
        d = Device::getFromList(devices, MQTT_MAX_NUM_OF_YOKIS_DEVICES, pch);
    }

    return d;
#else
    return currentDevice;
#endif
}

bool toggleCallback(const char* params) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    return g_bp->toggle();
}

bool onCallback(const char* params) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    return g_bp->on();
}

bool offCallback(const char* params) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        Serial.println("No such device");
        return false;
    }

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    return g_bp->off();
}

bool scannerCallback(const char*) {
    IrqManager::irqType = SCANNER;
    g_scanner->setDevice(currentDevice);
    g_scanner->setupRFModule();

    return true;
}

#ifdef ESP8266
bool storeConfigCallback(const char* param) {
    char paramBak[128];
    char* pch;
    bool ret;

    strncpy(paramBak, param, 127);
    pch = GenericCallback::getNextParam(paramBak);  // get the command 'save'
    pch = GenericCallback::getNextParam(NULL);  // Get the name of the device

    currentDevice->setDeviceName(pch);
    ret = currentDevice->saveToSpiffs();
    currentDevice->setDeviceName(CURRENT_DEVICE_DEFAULT_NAME);

    if (ret) Serial.println("Saved.");

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

bool displayDevices(const char*) {
    uint8_t c = 0;
    while (devices[c] != NULL) {
        Serial.println("=== Device ===");
        devices[c]->printDebugInfo();
        Serial.println("==============");
        c++;
    }

    return true;
}

bool reloadConfig(const char*) {
    for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
        delete(devices[i]); // delete previously allocated device if needed
        devices[i] = NULL;
    }
    Device::loadFromSpiffs(devices, MQTT_MAX_NUM_OF_YOKIS_DEVICES);
    Serial.println("Reloaded.");
    return true;
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    char* tok;
    char cBuf[32];
    Device* d;

    strncpy(cBuf, topic, 32);
    tok = strtok(cBuf, "/");
    d = Device::getFromList(devices, MQTT_MAX_NUM_OF_YOKIS_DEVICES, tok);

    unsigned int i = 0;
    for (i = 0; i < length; i++) {
        if(i < 31) {
            cBuf[i] = (char)payload[i];
        }
    }
    cBuf[i] = 0;

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    if (strcmp(cBuf, "ON") == 0) {
        g_bp->on();
        g_mqtt->notifyPower(d, "ON");
    } else if (strcmp(cBuf, "OFF") == 0) {
        g_bp->off();
        g_mqtt->notifyPower(d, "OFF");
    }
}
#endif
