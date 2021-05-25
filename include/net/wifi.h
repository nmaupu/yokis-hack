#ifdef ESP8266
#ifndef __WIFI_H__
#define __WIFI_H__

#include <ESP8266WiFi.h>

int reconnectWifi();
void setupWifi();
void setupWifi(const String ssid, const String password);
void setupWifiAP();
bool resetWifiConfig();

#endif  // __WIFI_H__
#endif  // ESP8266
