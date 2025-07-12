#ifdef WIFI_ENABLED
#ifdef ESP8266
#include "net/wifi.h"
#include "globals.h"

void setupWifi(String ssid, String password) {
    // Configuration changed
    if (strcmp(WiFi.SSID().c_str(), ssid.c_str()) != 0 ||
        strcmp(WiFi.psk().c_str(), password.c_str()) != 0) {

        WiFi.persistent(true);
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
        WiFi.setAutoReconnect(true);
        WiFi.setAutoConnect(true);
        WiFi.persistent(false);

        WiFi.begin(ssid.c_str(), password.c_str(), 0, NULL, true);
    } else {
        // Since arduino core v3, wifi boots with radio set to OFF
        // Need to reconnect when calling setupWifi at boot when wifi
        // is configured using macros at compile time.
        reconnectWifi();
    }
}

int reconnectWifi() {
    //WiFi.disconnect(true); // this makes wifi not connecting somehow
    WiFi.persistent(true);
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.setAutoReconnect(true);
    WiFi.setAutoConnect(true);
    WiFi.reconnect();
    WiFi.persistent(false);

    uint8_t c = 0;

    LOG.print("Connecting to WiFi ");
    while (c < 100) {
        LOG.print(".");
        c++;
        if (WiFi.status() == WL_CONNECTED) {  // ok
            LOG.print(" connected to SSID ");
            LOG.println(WiFi.SSID());
            return WiFi.status();
        }
        delay(50);
    }

    LOG.println(" unable to connect to wifi.");
    return WiFi.status();
}

void setupWifi() {
    if (reconnectWifi() == WL_CONNECTED) {
        LOG.println("WiFi already connected, setup skipped.");
        return;
    }

    if (strlen(WiFi.SSID().c_str()) > 0) {
        // SSID is set but wrong password or wifi down
        // Not using AP mode, retrying instead
        LOG.println("Failed. WiFi SSID is already set, retrying. (wrong password? wifi down?)");
        return;
    }

    // Wifi not configured
    // setting up an AP for initial configuration
    LOG.println("Connection timed out, using AP mode.");
    setupWifiAP();
}

void setupWifiAP() {
    WiFi.persistent(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    delay(100);
    // Scan network here

    String ssid = "YokisHack-";
    ssid += String(ESP.getFlashChipId(), HEX);
    WiFi.softAP(ssid, "");
    WiFi.mode(WIFI_AP);
    WiFi.waitForConnectResult();
    WiFi.persistent(false);

    LOG.print("WiFi AP mode started. SSID: ");
    LOG.println(ssid);
    LOG.print("YokisHack IP: ");
    LOG.println(WiFi.softAPIP());
}

bool resetWifiConfig() {
    WiFi.persistent(true);
    WiFi.disconnect(true);
    ESP.eraseConfig();
    WiFi.persistent(false);
    return WiFi.status() == WL_DISCONNECTED && WiFi.SSID().isEmpty();
}

#endif
#endif
