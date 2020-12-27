#ifdef ESP8266
#include "net/wifi.h"
#include "globals.h"

#define WIFI_CONNECT_MAX_TRIES 5

void setupWifi() {
    // temporarily resetting wifi config to force testing AP mode
    resetWifiConfig();
    WiFi.mode(WIFI_STA);

    uint8_t c = 0;

    LOG.print("Connecting to WiFi ");
    while (c < 5) {
        LOG.print(".");
        c++;
        if (WiFi.status() == WL_CONNECTED) {  // ok
            LOG.print(" connected to SSID ");
            LOG.println(WiFi.SSID());
            return;
        }
        delay(500);
    }

    if (strlen(WiFi.SSID().c_str()) > 0) {
        // SSID is set but wrong password or wifi down
        // Not using AP mode, retrying instead
        LOG.println("WiFi SSID is already set, retrying.");
        return;
    }

    // Wifi not configured
    // setting up an AP for initial configuration
    LOG.println(" connection timed out, using AP mode.");
    setupWifiAP();
}

void setupWifiAP() {
    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    delay(100);
    // Scan network here

    String ssid = "YokisHack-";
    ssid += String(random(0xffff), HEX);
    WiFi.softAP(ssid, "");
    WiFi.mode(WIFI_AP);
    WiFi.persistent(true);
    WiFi.waitForConnectResult();

    Serial.print("WiFi AP mode started. SSID: ");
    Serial.println(ssid);
    Serial.print("YokisHack IP: ");
    Serial.println(WiFi.softAPIP());
}

bool resetWifiConfig() {
    WiFi.persistent(false);
    WiFi.disconnect();
    return WiFi.status() == WL_DISCONNECTED;
}

#endif
