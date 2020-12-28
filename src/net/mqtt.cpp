#ifdef ESP8266
#include "net/mqtt.h"
#include <Arduino.h>
#include "globals.h"

Mqtt::Mqtt(WiFiClient& wifiClient) : PubSubClient(wifiClient), MqttConfig() {}

Mqtt::Mqtt(WiFiClient& wifiClient, const char* host, const uint16_t port,
           const char* username, const char* password)
    : PubSubClient(wifiClient), MqttConfig(host, port, username, password) {
    this->setCallback(Mqtt::callback);

    // Init subscriptions to NULL
    subscribedTopicIdx = 0;
    for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
        subscribedTopics[i] = NULL;
    }
}

void Mqtt::setConnectionInfo(const char* host, const uint16_t port, const char* username, const char* password) {
    // String memory management is handled by the (operator =) func override
    this->MqttConfig::host = host;
    this->MqttConfig::port = port;
    this->MqttConfig::username = username;
    this->MqttConfig::password = password;

    if(this->connected()) {
        this->disconnect();
    }
    this->setServer(host, port);
    this->reconnect();
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
    // No configuration available
    if (this->MqttConfig::empty()) {
        return false;
    }

    char buf[128];
    String clientId = "YokisHack-";
    clientId += String(random(0xffff), HEX);

    sprintf(buf, "Connecting to MQTT %s:%hu with client ID=%s... ", host.c_str(), this->MqttConfig::port,
            clientId.c_str());
    LOG.print(buf);

    if (this->connect(clientId.c_str(), username.c_str(), password.c_str())) {
        LOG.println("connected");
        this->resubscribe();  // resubscribe to all configured topics
    } else {
        LOG.print("failed with state ");
        LOG.println(this->state());
    }

    return this->connected();
}

boolean Mqtt::loop() {
    if (this->MqttConfig::empty()) {
        return false;
    }

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
