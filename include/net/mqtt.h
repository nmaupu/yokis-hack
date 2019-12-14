#ifdef ESP8266
#ifndef __MQTT_H__
#define __MQTT_H__

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MAX_NUMBER_OF_YOKIS_DEVICES 255

class Mqtt : public PubSubClient {
   private:
    const char* username;
    const char* password;
    const char* host;
    const uint16_t* port;
    char* subscribedTopics[MAX_NUMBER_OF_YOKIS_DEVICES];
    uint8_t subscribedTopicIdx;
    void resubscribe();

   public:
    Mqtt(WiFiClient&, const char*, const uint16_t*, const char*, const char*);
    boolean subscribe(const char*);
    void clearSubscriptions();
    void reconnect();
    boolean loop();
    static void callback(char*, uint8_t*, unsigned int);
};

#endif
#endif