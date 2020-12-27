#ifdef ESP8266
#ifndef __WIFI_H__
#define __WIFI_H__

#include <EEPROM.h>
#include <ESP8266WiFi.h>

void setupWifi();
void setupWifiAP();

#endif  // __WIFI_H__
#endif  // ESP8266
