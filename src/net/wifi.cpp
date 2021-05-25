#ifdef ESP8266
#include "net/wifi.h"
#include "globals.h"

void setupWifi(String ssid, String password) {
    // Configuration changed
    if (strcmp(WiFi.SSID().c_str(), ssid.c_str()) != 0 ||
        strcmp(WiFi.psk().c_str(), password.c_str()) != 0) {

        WiFi.disconnect(true);
        WiFi.persistent(true);
        WiFi.mode(WIFI_STA);
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
        WiFi.setAutoReconnect(true);
        ETS_UART_INTR_DISABLE();
        wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();

        WiFi.begin(ssid.c_str(), password.c_str(), 0, NULL, true);
    }
}

int reconnectWifi() {
    //WiFi.disconnect(true); // this makes wifi not connecting somehow
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.setAutoReconnect(true);
    WiFi.reconnect();


    uint8_t c = 0;

    LOG.print("Connecting to WiFi ");
    while (c < 10) {
        LOG.print(".");
        c++;
        if (WiFi.status() == WL_CONNECTED) {  // ok
            LOG.print(" connected to SSID ");
            LOG.println(WiFi.SSID());
            return WiFi.status();
        }
        delay(500);
    }

    LOG.println(" unable to connect to wifi.");
    return WiFi.status();
}

void setupWifi() {
    if (reconnectWifi() == WL_CONNECTED) {
        return;
    }

    if (strlen(WiFi.SSID().c_str()) > 0) {
        // SSID is set but wrong password or wifi down
        // Not using AP mode, retrying instead
        LOG.println("Failed. WiFi SSID is already set, retrying.");
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
    ssid += String(ESP.getFlashChipId(), HEX);
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
    WiFi.persistent(true);
    WiFi.disconnect(true);
    return WiFi.status() == WL_DISCONNECTED;
}

#endif
