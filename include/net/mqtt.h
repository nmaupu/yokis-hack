#if WIFI_ENABLED && (defined(ESP8266) || defined(ESP32)) && MQTT_ENABLED
#ifndef __MQTT_H__
#define __MQTT_H__

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include <PubSubClient.h>
#include "net/mqttConfig.h"
#include "constants.h"

#define MQTT_TOPIC_COMMAND "cmnd"
#define MQTT_CONNECT_RETRY_EVERY_MS 2000
// #ifndef MAX_YOKIS_DEVICES_NUM
// #define MAX_YOKIS_DEVICES_NUM 64
// #endif

class Mqtt : public PubSubClient, public MqttConfig {
   private:
    ulong lastConnectionRetry = 0UL;
    char* subscribedTopics[MAX_YOKIS_DEVICES_NUM];
    uint8_t subscribedTopicIdx;
    void resubscribe();
    static void callback(char*, uint8_t*, unsigned int);

   public:
    Mqtt(WiFiClient&);
    Mqtt(WiFiClient&, MqttConfig&);
    ~Mqtt();
    void setConnectionInfo(MqttConfig&, bool=true);
    void setConnectionInfo(const char*, uint16_t, const char*, const char*, bool=true);
    boolean subscribe(const char*);
    void clearSubscriptions();
    bool reconnect(bool=false);
    boolean loop();
};

#endif
#endif
