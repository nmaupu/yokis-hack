#ifdef ESP8266
#include "net/wifi.h"

void setupWifi() {
#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
    Serial.print("Connecting to SSID ");
    Serial.print(WIFI_SSID);
    Serial.println(" ...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
        delay(500);
        Serial.print('.');
    }
    Serial.println();
    Serial.println("Connection established!");
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
#else
    Serial.println("WiFi parameters are not defined. Cannot configure WiFi !");
#endif
}

#endif
