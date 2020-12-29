#ifdef ESP8266
#ifndef __MQTT_CONFIG_H__
#define __MQTT_CONFIG_H__

#include <LittleFS.h>
#include <WString.h>
#include <stdlib.h>

#include "storage/yokisLittleFS.h"

#define MQTT_DEFAULT_PORT       1883
#define MQTT_CONFIG_FILE_NAME   "/mqtt.conf"

class MqttConfig {
    protected:
     String host;
     uint16_t port;
     String username;
     String password;

    public:
     MqttConfig();
     MqttConfig(const MqttConfig& config);
     MqttConfig(const char*, const uint16_t, const char*, const char*);
     void setHost(String);
     void setPort(uint16_t);
     void setUsername(String);
     void setPassword(String);
     String getHost();
     uint16_t getPort();
     String getUsername();
     String getPassword();
     static MqttConfig loadConfig();
     bool saveConfig();
     bool isEmpty();
     void printDebug(Print&);

     // Configuration persistence
     bool saveToLittleFS();
     static MqttConfig loadFromLittleFS();

};

#endif  // __MQTT_CONFIG_H__
#endif  // ESP8266
