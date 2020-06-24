#ifdef ESP8266
#ifndef __MQTT_H__
#define __MQTT_H__

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MQTT_TOPIC_COMMAND "cmnd"
#define MQTT_MAX_NUM_OF_YOKIS_DEVICES 64
#define MQTT_CONNECT_MAX_RETRIES 3

class Mqtt : public PubSubClient {
   private:
    const char* username;
    const char* password;
    const char* host;
    const uint16_t* port;
    uint8_t connectRetries;
    char* subscribedTopics[MQTT_MAX_NUM_OF_YOKIS_DEVICES];
    uint8_t subscribedTopicIdx;
    void resubscribe();
    static void callback(char*, uint8_t*, unsigned int);

   public:
    Mqtt(WiFiClient&, const char*, const uint16_t*, const char*, const char*);
    boolean subscribe(const char*);
    void clearSubscriptions();
    void reconnect();
    boolean loop();
};

#endif
#endif
