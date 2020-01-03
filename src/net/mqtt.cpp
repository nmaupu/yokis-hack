#ifdef ESP8266
#include "net/mqtt.h"
#include <Arduino.h>

Mqtt::Mqtt(WiFiClient& wifiClient, const char* host, const uint16_t* port,
           const char* username, const char* password)
    : PubSubClient(wifiClient) {
    this->host = host;
    this->port = port;
    this->username = username;
    this->password = password;
    this->setServer(host, *port);
    this->setCallback(Mqtt::callback);

    // Init subscriptions to NULL
    subscribedTopicIdx = 0;
    for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
        subscribedTopics[i] = NULL;
    }
}

boolean Mqtt::subscribe(const char* topic) {
    if (subscribedTopicIdx >= MQTT_MAX_NUM_OF_YOKIS_DEVICES) return false;

    subscribedTopics[subscribedTopicIdx] = (char*)malloc(strlen(topic)+1);
    strlcpy(subscribedTopics[subscribedTopicIdx], topic, strlen(topic)+1);
    subscribedTopicIdx++;

    return PubSubClient::subscribe(topic);
}

void Mqtt::resubscribe() {
    for(uint8_t i=0; i<subscribedTopicIdx; i++) {
        PubSubClient::subscribe(subscribedTopics[i]);
    }
}

void Mqtt::clearSubscriptions() {
    for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
        if (subscribedTopics[i] != NULL) {
            Serial.println(subscribedTopics[i]);
            free(subscribedTopics[i]);
        }
    }

    subscribedTopicIdx = 0;
}

void Mqtt::reconnect() {
    char buf[128];
    while (!this->connected()) {
        sprintf(buf, "Connecting to MQTT %s:%hu ...", host, *port);
        Serial.println(buf);

        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);

        if (this->connect("ESP8266Client", username, password)) {
            Serial.println("connected");
            this->resubscribe(); // resubscribe to all configured topics
        } else {
            Serial.print("failed with state ");
            Serial.print(this->state());
            delay(5000);
        }
    }
}

boolean Mqtt::loop() {
    if (!this->connected()) {
        this->reconnect();
    }
    // Call parent function
    return PubSubClient::loop();
}

void Mqtt::callback(char* topic, uint8_t* payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);

    Serial.print("Message:");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }

    Serial.println();
    Serial.println("-----------------------");
}

#endif
