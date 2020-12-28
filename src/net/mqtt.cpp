#ifdef ESP8266
#include "net/mqtt.h"
#include <Arduino.h>
#include "globals.h"

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
            LOG.println(subscribedTopics[i]);
            free(subscribedTopics[i]);
        }
    }

    subscribedTopicIdx = 0;
}

bool Mqtt::reconnect() {
    char buf[128];
    String clientId = "YokisHack-";
    clientId += String(random(0xffff), HEX);

    sprintf(buf, "Connecting to MQTT %s:%hu with client ID=%s... ", host, *port,
            clientId.c_str());
    LOG.print(buf);

    if (this->connect(clientId.c_str(), username, password)) {
        LOG.println("connected");
        this->resubscribe();  // resubscribe to all configured topics
    } else {
        LOG.print("failed with state ");
        LOG.println(this->state());
    }

    return this->connected();
}

boolean Mqtt::loop() {
    if(!this->connected()) {
        for(uint8_t i=0; i<MQTT_CONNECT_MAX_RETRIES; i++) {
            if(this->reconnect()) {
                break;
            } else {
                delay(2000);
            }
        }
    }

    // Call parent function
    return PubSubClient::loop();
}

// static - default callback logging on Serial
void Mqtt::callback(char* topic, uint8_t* payload, unsigned int length) {
    LOG.print("Message topic: ");
    LOG.println(topic);

    LOG.print("Message payload: ");
    for (unsigned int i = 0; i < length; i++) {
        LOG.print((char)payload[i]);
    }

    LOG.println();
    LOG.println("-----------------------");
}

#endif
