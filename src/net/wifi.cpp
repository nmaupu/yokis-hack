#ifdef ESP8266
#include "net/wifi.h"
#include "globals.h"

void setupWifi() {
#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
    LOG.print("Connecting to SSID ");
    LOG.print(WIFI_SSID);
    LOG.println(" ...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
        delay(500);
        LOG.print('.');
    }
    LOG.println();
    LOG.println("Connection established!");
    LOG.print("IP address:\t");
    LOG.println(WiFi.localIP());
#else
    LOG.println("WiFi parameters are not defined. Cannot configure WiFi !");
#endif
}

#endif
