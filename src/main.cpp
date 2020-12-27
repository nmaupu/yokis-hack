#include <Arduino.h>

#include "RF/copy.h"
#include "RF/e2bp.h"
#include "RF/irqManager.h"
#include "RF/pairing.h"
#include "RF/scanner.h"
#include "commands/callbacks.h"
#include "globals.h"
#include "printf.h"
#include "serial/serialHelper.h"

#ifdef ESP8266
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <WiFiUdp.h>
#include "net/mqttHass.h"
#include "net/wifi.h"
#include "net/webserver.h"
#endif

// static irqType initialization
IrqType IrqManager::irqType = PAIRING;

// globals' initialization
byte g_ConfigFlags = ~FLAG_RAW & ~FLAG_DEBUG & FLAG_POLLING;
SerialHelper* g_serial;
Pairing* g_pairingRF;
E2bp* g_bp;
Scanner* g_scanner;
Copy* g_copy;
Device* g_currentDevice;

#ifdef ESP8266
#ifdef MQTT_ENABLED
MqttHass* g_mqtt;
#endif // MQTT_ENABLED
TelnetSpy g_telnetAndSerial;
// no need to store more devices than supported by MQTT
Device* g_devices[MQTT_MAX_NUM_OF_YOKIS_DEVICES];
#endif // ESP8266

//
#ifdef ESP8266

WebServer webserver(80);
Ticker* g_deviceStatusPollers[MQTT_MAX_NUM_OF_YOKIS_DEVICES];
WiFiClient espClient;

// polling
void pollForStatus(Device* device);

#ifdef MQTT_ENABLED
void mqttCallback(char*, uint8_t*, unsigned int);
#endif // MQTT_ENABLED

#endif // ESP8266

// Setup inits everything: singletons and commands' callback
void setup() {
    randomSeed(micros());

    // Globals' initialization
    g_serial = new SerialHelper();
    g_pairingRF = new Pairing(CE_PIN, CSN_PIN);
    g_bp = new E2bp(CE_PIN, CSN_PIN);
    g_scanner = new Scanner(CE_PIN, CSN_PIN);
    g_copy = new Copy(CE_PIN, CSN_PIN);
    g_currentDevice = new Device(CURRENT_DEVICE_DEFAULT_NAME);

#ifdef ESP8266
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, HIGH);  // pin is inverted so, set it off

    // Load all previously stored devices from LittleFS memory
    reloadConfig(NULL);

    // Setting up configured wifi or AP mode
    // If compilation options are present, override any existing configuration
    #ifdef WIFI_SSID
        String ssid = WIFI_SSID;
        String psk = "";
        #ifdef WIFI_PASSWORD
        psk = WIFI_PASSWORD;
        #endif // WIFI_PASSWORD
        LOG.print("WIFI_SSID is set, forcing this configuration. SSID=");
        LOG.println(WIFI_SSID);
        setupWifi(ssid, psk);
    #else
        setupWifi(); // Setup existing configuration of set AP mode for initial config
    #endif // WIFI_SSID

    // Starting webserver
    webserver.begin();

    #if defined(MQTT_ENABLED)
    g_mqtt = new MqttHass(espClient);
    g_mqtt->setCallback(mqttCallback);
    #endif

    // OTA
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {  // U_FS
            type = "filesystem";
        }
        LOG.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() { LOG.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        LOG.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        LOG.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            LOG.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            LOG.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            LOG.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            LOG.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            LOG.println("End Failed");
        }
    });
    ArduinoOTA.begin();
#endif

    // Registering all commands
    registerAllCallbacks();

    // Handle interrupt pin
    pinMode(IRQ_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(IRQ_PIN), IrqManager::processIRQ, FALLING);

    printf_begin();  // Works only for Arduino devices...
    LOG.println("Setup finished - device ready !");
    g_serial->executeCallback("help");
    LOG.println();
    g_serial->prompt();
}

void loop() {
#if defined(ESP8266)
    LOG.handle(); // telnetspy handling
    ArduinoOTA.handle();

    #if defined(MQTT_ENABLED)
    bool mqttLoop = g_mqtt->loop();
    if (!mqttLoop && g_mqtt->isDiscoveryDone()) {
        // loop is faulty, network is down ?
        setupWifi();
    }

    uint8_t nbDevices = 0;
    if (g_mqtt->connected()  && !g_mqtt->isDiscoveryDone()) {
        LOG.print("Publishing homeassistant discovery data... ");
        for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
            if (g_devices[i] != NULL) {
                nbDevices++;
                if (g_mqtt->publishDevice(g_devices[i])) {
                    g_mqtt->subscribeDevice(g_devices[i]);
                } else {
                    LOG.println("KO");
                    break;
                }
            }
        }

        if (nbDevices == 0) g_mqtt->setDiscoveryDone(true);

        if (g_mqtt->isDiscoveryDone()) LOG.println("OK");

    } else if (g_mqtt->isDiscoveryDone()) {
        // Verify polling statuses and update via MQTT if needed
        for (uint8_t i = 0;
             i < MQTT_MAX_NUM_OF_YOKIS_DEVICES && FLAG_IS_ENABLED(FLAG_POLLING);
             i++) {
            if (g_devices[i] != NULL && g_devices[i]->needsPolling()) {
                pollForStatus(g_devices[i]);
            }
        }
    }
#endif // MQTT_ENABLED
#endif // ESP8266
    g_serial->readFromSerial();
    delay(1);
}


#if defined(ESP8266) && defined(MQTT_ENABLED)
void pollForStatus(Device* d) {
    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    DeviceStatus ds = g_bp->pollForStatus();

    if (ds != UNDEFINED) {  // device reachable
        if (d->getFailedPollings() > 0) {
            LOG.print("Device ");
            LOG.print(d->getName());
            LOG.println(" recovered");
        }

        d->pollingSuccess();

        if (d->isOffline()) {  // Device is back online
            d->online();
            g_mqtt->notifyOnline(d);
        }

        // Update device status - even if unchanged
        // Hence, in case of hass restart, status are updated
        d->setStatus(ds);
        if (d->getMode() == DIMMER) {
            if (ds == ON && d->getBrightness() == 0)
                d->setBrightness(BRIGHTNESS_MAX);
            g_mqtt->notifyBrightness(d);
        } else {
            g_mqtt->notifyPower(d);
        }
    } else {
        if (d->pollingFailed() >= DEVICE_MAX_FAILED_POLLING_BEFORE_OFFLINE) {
            // Device is unreachable
            LOG.print("Device ");
            LOG.print(d->getName());
            LOG.println(" is offline");
            d->offline();
            g_mqtt->notifyOffline(d);
        } else {
            LOG.print("Failed to check device ");
            LOG.print(d->getName());
            LOG.print(" ");
            LOG.print(d->getFailedPollings(), DEC);
            LOG.print("/");
            LOG.println(DEVICE_MAX_FAILED_POLLING_BEFORE_OFFLINE, DEC);
        }
    }
}
#endif

#if defined(ESP8266) && defined(MQTT_ENABLED)
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
    d = Device::getFromList(g_devices, MQTT_MAX_NUM_OF_YOKIS_DEVICES, tok);

    // If we update this device too soon, ignore the payload
    unsigned long now = millis();
    if (d->getLastUpdateMillis() + MQTT_UPDATE_MILLIS_WINDOW > now) {
        LOG.println(
            "Ignoring MQTT message: received too soon for this device");
        LOG.print("Last update: ");
        LOG.println(d->getLastUpdateMillis(), DEC);
        LOG.print("This update: ");
        LOG.println(now, DEC);
        LOG.print("Difference: ");
        LOG.println(now - d->getLastUpdateMillis());

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
        case SHUTTER:
        case NO_RCPT:
            if (strcmp(mPayload, "ON") == 0) {
                g_bp->on();
            } else if (strcmp(mPayload, "OFF") == 0) {
                g_bp->off();
            } else if (strcmp(mPayload, "PAUSE") == 0) {
                g_bp->pauseShutter();
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
#endif  // #if defined(ESP8266) && defined(MQTT_ENABLED)
