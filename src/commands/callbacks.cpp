#include "commands/callbacks.h"

#include "globals.h"

void registerAllCallbacks() {
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
        "pause", "Pause the configured device (MVR500 only - shutter device)",
        pauseShutterCallback));
    g_serial->registerCallback(new GenericCallback(
        "press", "Press and hold an e2bp button", pressCallback));
    g_serial->registerCallback(new GenericCallback(
        "pressFor", "Press and hold for x milliseconds", pressForCallback));
    g_serial->registerCallback(new GenericCallback(
        "release", "Release an e2bp button", releaseCallback));
    g_serial->registerCallback(
        new GenericCallback("status", "Get device status", statusCallback));
    g_serial->registerCallback(
        new GenericCallback("statusAll", "Get status for all devices", statusAllCallback));
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
        "save", "Save current device configuration to LittleFS",
        storeConfigCallback));
    g_serial->registerCallback(new GenericCallback(
        "delete", "Delete one entry from LittleFS configuration",
        deleteFromConfig));
    g_serial->registerCallback(new GenericCallback(
        "clear", "Clear all config previously stored to LittleFS",
        clearConfig));
    g_serial->registerCallback(new GenericCallback(
        "reload", "Reload config from LittleFS to memory", reloadConfig));
    g_serial->registerCallback(new GenericCallback(
        "dConfigFS", "display config previously stored in LittleFS",
        displayConfig));
    g_serial->registerCallback(new GenericCallback(
        "dRestore",
        "restore a previously saved raw config line (SPIFFS->LittleFS)",
        restoreConfig));
    #if WIFI_ENABLED
    g_serial->registerCallback(
        new GenericCallback("wifiConfig",
                            "Configure wifi with parameters: ssid psk (does "
                            "not work for psk containing spaces)",
                            wifiConfig));
    g_serial->registerCallback(
        new GenericCallback("wifiReconnect",
                            "Attempt to reconnect wifi with current config",
                            wifiReconnect));
    g_serial->registerCallback(new GenericCallback(
        "wifiDiag", "Display wifi configuration debug info", wifiDiag));
    g_serial->registerCallback(new GenericCallback(
        "wifiReset", "Reset wifi configuration and setup AP mode",
        resetWifiConfigCallback));

    #if MQTT_ENABLED
        g_serial->registerCallback(
            new GenericCallback("mqttConfig",
                                "Configure MQTT options (format: mqttConfig host "
                                "port username password)",
                                mqttConfig));
        g_serial->registerCallback(new GenericCallback(
            "mqttDiag", "Display current MQTT configuration", mqttDiag));
        g_serial->registerCallback(new GenericCallback(
            "mqttConfigDelete", "Delete current MQTT configuration",
            mqttConfigDelete));
    #endif // MQTT_ENABLED
    #endif // WIFI_ENABLED

    g_serial->registerCallback(
        new GenericCallback("restart", "Restart the ESP8266 board", restart));
    g_serial->registerCallback(
        new GenericCallback("reboot", "Restart the ESP8266 board", restart));

#endif // ESP8266
}

bool pairingCallback(const char*) {
    uint8_t buf[5];  // enough size for addr, serial and version
    IrqManager::irqType = PAIRING;
    bool res = g_pairingRF->hackPairing();
    if (res) {
        g_pairingRF->getAddressFromRecvData(buf);
        g_currentDevice->setHardwareAddress(buf);
        g_currentDevice->setChannel(g_pairingRF->getChannelFromRecvData());

        g_pairingRF->getSerialFromRecvData(buf);
        g_currentDevice->setSerial(buf);

        g_pairingRF->getVersionFromRecvData(buf);
        g_currentDevice->setVersion(buf);

        // Get device status and get device mode
        statusCallback(NULL);
        g_currentDevice->setMode(g_bp->getDeviceModeFromRecvData());

        g_currentDevice->toSerial();
    }
    return res;
}

// Get a device from the list with the given params
Device* getDeviceFromParams(const char* params) {
#ifdef ESP8266
    if (params == NULL || strcmp("", params) == 0) return g_currentDevice;

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
        d = g_currentDevice;
    } else {
        d = Device::getFromList(g_devices, g_nb_devices, pch);
    }

    delete[] paramsBak;
    return d;
#else
    return g_currentDevice;
#endif
}

// Generic on, off or toggle
bool changeDeviceState(const char* params, bool (E2bp::*func)(void)) {
    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        LOG.println("No such device");
        return false;
    }

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    bool ret = (g_bp->*func)();
#if defined(ESP8266) && MQTT_ENABLED
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

bool pauseShutterCallback(const char* params) {
    return changeDeviceState(params, &E2bp::pauseShutter);
}

bool scannerCallback(const char* params) {
    if (FLAG_IS_ENABLED(FLAG_POLLING)) {
        LOG.println("Disable polling before attempting to scan ! Aborting.");
        return false;
    }

    Device* d = getDeviceFromParams(params);

    if (d == NULL || d->getHardwareAddress() == NULL) {
        LOG.println("No such device");
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
        LOG.println("No such device");
        return false;
    }

    IrqManager::irqType = COPYING;
    g_copy->setDevice(d);
    return g_copy->send();
}

bool displayDevices(const char*) {
#ifdef ESP8266
    uint8_t c = 0;
    while (g_devices[c] != NULL) {
        LOG.println("=== Device ===");
        g_devices[c]->toSerial();
        LOG.println("==============");
        c++;
    }
#else
    // for arduino devices, only display the g_currentDevice
    g_currentDevice->toSerial();
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
        LOG.println("No such device");
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
        LOG.println("No such device");
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
        LOG.println("No such device");
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
        LOG.println("No such device");
        return false;
    }

    IrqManager::irqType = E2BP;
    g_bp->setDevice(d);
    DeviceStatus st = g_bp->pollForStatus();

    LOG.print("Device Status = ");
    LOG.println(Device::getStatusAsString(st));

    return true;
}

bool statusAllCallback(const char* params) {
    Device* d;
    IrqManager::irqType = E2BP;
    for (uint8_t i = 0; i < g_nb_devices; i++) {
        d = g_devices[i];
        if (d != NULL) {
            g_bp->setDevice(d);
            DeviceStatus st = g_bp->pollForStatus();
            LOG.printf("%s status = %s\n", d->getName(), Device::getStatusAsString(st));
        }
    }

    if (g_nb_devices > 0) LOG.flush();

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
    if(pch == NULL || strlen(pch) == 0) {
        LOG.println("Cannot save. Please specify a name for this device!");
        return false;
    }
    g_currentDevice->setName(pch);

    // This should be auto detected from pairing
    pch = strtok(NULL, " ");  // Get the device mode
    if (pch != NULL) {
        g_currentDevice->setMode(pch);
    }

    ret = g_currentDevice->saveToLittleFS();
    if (ret) LOG.println("Saved.");

    // reset default name
    g_currentDevice->setName(CURRENT_DEVICE_DEFAULT_NAME);

    reloadConfig(params);

    delete[] paramsBak;
    return ret;
}

bool clearConfig(const char*) {
    Device::clearConfigFromLittleFS();
    return true;
}

bool displayConfig(const char*) {
    Device::displayConfigFromLittleFS();
    return true;
}

bool restoreConfig(const char* params) {
    char* paramsBak;
    char* pch;

    int len = strlen(params);
    paramsBak = new char[len + 1];
    strncpy(paramsBak, params, len);
    paramsBak[len] = 0;
    strtok(paramsBak, " ");   // ignore the command
    pch = strtok(NULL, " ");  // Get the line to restore

    bool ret = Device::storeRawConfig(pch);

    delete[] paramsBak;
    return ret;
}

bool reloadConfig(const char*) {
    for (uint8_t i = 0; i < MAX_YOKIS_DEVICES_NUM; i++) {
        delete g_devices[i];  // delete previously allocated device if needed
        if (g_deviceStatusPollers[i] != NULL) g_deviceStatusPollers[i]->detach();
        delete g_deviceStatusPollers[i];
        g_devices[i] = NULL;
        g_deviceStatusPollers[i] = NULL;
    }
    g_nb_devices = Device::loadFromLittleFS(g_devices, MAX_YOKIS_DEVICES_NUM);

    // Reattach tickers to devices
    for (uint8_t i = 0; i < g_nb_devices; i++) {
        if (g_devices[i] != NULL) {
            g_deviceStatusPollers[i] = new Ticker();
            g_deviceStatusPollers[i]->attach_ms(random(4000, 10000), pollDevice,
                                              g_devices[i]);
        }
    }
    LOG.println("Reloaded.");
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


#if WIFI_ENABLED
bool resetWifiConfigCallback(const char* params) {
    return resetWifiConfig();
}

bool wifiConfig(const char* params) {
    char* paramsBak;
    char* ssid;
    char* psk;

    int len = strlen(params);
    paramsBak = new char[len + 1];
    strncpy(paramsBak, params, len);
    paramsBak[len] = 0;
    strtok(paramsBak, " ");    // ignore the command name
    ssid = strtok(NULL, " ");  // Get the ssid
    psk = strtok(NULL, " ");   // Get the psk if set

    setupWifi(ssid, psk);

    delete[] paramsBak;
    return true;
}

bool wifiReconnect(const char* params) {
    reconnectWifi();
    return true;
}

bool wifiDiag(const char* params) {
    WiFi.printDiag(LOG);

    LOG.print("Yokis-Hack IP: ");
    if (WiFi.getMode() == WIFI_AP) {
        LOG.println(WiFi.softAPIP());
    } else {
        LOG.println(WiFi.localIP());
    }

    return true;
}
#endif // WIFI_ENABLED

bool restart(const char* params) {
    ESP.restart();
    return true;
}

#if MQTT_ENABLED
bool mqttConfig(const char* params) {
    char* paramsBak;
    char *host, *sport, *username, *password;
    MqttConfig config;

    int len = strlen(params);
    paramsBak = new char[len + 1];
    strncpy(paramsBak, params, len);
    paramsBak[len] = 0;
    strtok(paramsBak, " ");    // Ignore the command name
    host = strtok(NULL, " ");  // Get the host
    if (host == NULL) {
        LOG.println("MQTT host cannot be null. Aborting.");
        return false;
    }
    config.setHost(host);

    sport = strtok(NULL, " ");  // Get the port
    if (sport == NULL || strlen(sport) == 0 || strlen(sport) > 5) {
        LOG.println("MQTT port is null or too high. Aborting.");
        return false;
    }
    config.setPort((uint16_t)atol(sport));

    username = strtok(NULL, " ");  // Get the username
    config.setUsername(username);

    password = strtok(NULL, " ");  // Get the password
    config.setPassword(password);

    LOG.println("MQTT configuration:");
    config.printDebug(LOG);

    g_mqtt->setDiscoveryDone(false);
    g_mqtt->setConnectionInfo(config);

    delete[] paramsBak;
    return true;
}

bool mqttDiag(const char* params) {
    LOG.println("Current MQTT configuration:");
    g_mqtt->printDebug(LOG);
    return true;
}

bool mqttConfigDelete(const char*) {
    MqttConfig emptyConfig;
    MqttConfig::deleteConfigFromLittleFS();
    g_mqtt->setConnectionInfo(emptyConfig);
    LOG.println("MQTT config deleted!");
    return true;
}

#endif // MQTT_ENABLED
#endif // ESP8266
