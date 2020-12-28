#ifdef ESP8266
#ifndef __MQTT_CONFIG_H__
#define __MQTT_CONFIG_H__

#include <WString.h>

#define MQTT_DEFAULT_PORT 1883

class MqttConfig {
    protected:
     String host;
     uint16_t port;
     String username;
     String password;

    public:
     MqttConfig();
     MqttConfig(const char*, const uint16_t, const char*, const char*);
     void setHost(const char*);
     void setPort(const uint16_t);
     void setUsername(const char*);
     void setPassword(const char*);
     static MqttConfig loadConfig();
     bool saveConfig();
     bool empty();
};

#endif  // __MQTT_CONFIG_H__
#endif  // ESP8266
