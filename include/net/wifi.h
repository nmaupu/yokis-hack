#if WIFI_ENABLED && (defined(ESP8266) || defined(ESP32))
#ifndef __WIFI_H__
#define __WIFI_H__

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

int reconnectWifi();
void setupWifi();
void setupWifi(const String ssid, const String password);
void setupWifiAP();
bool resetWifiConfig();

#endif  // __WIFI_H__
#endif  // ESP8266 || ESP32
