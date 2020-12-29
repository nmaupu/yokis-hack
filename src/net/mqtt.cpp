#ifdef ESP8266
#include "net/mqtt.h"
#include <Arduino.h>
#include "globals.h"

Mqtt::Mqtt(WiFiClient& wifiClient) : PubSubClient(wifiClient), MqttConfig() {}

Mqtt::Mqtt(WiFiClient& wifiClient, MqttConfig& mqttConfig)
    : PubSubClient(wifiClient), MqttConfig(mqttConfig) {
    this->setCallback(Mqtt::callback);

    // Init subscriptions to NULL
    subscribedTopicIdx = 0;
    for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
        subscribedTopics[i] = NULL;
    }
}

void Mqtt::setConnectionInfo(MqttConfig& config) {
    setConnectionInfo(config.getHost().c_str(),
                      config.getPort(),
                      config.getUsername().c_str(),
                      config.getPassword().c_str());
}

void Mqtt::setConnectionInfo(const char* host, uint16_t port, const char* username, const char* password) {
    this->setHost(host);
    this->setPort(port);
    this->setUsername(username);
    this->setPassword(password);
    this->MqttConfig::saveToLittleFS();

    if (this->connected()) {
        this->disconnect();
    }
    this->setServer(getHost().c_str(), getPort());
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
    if (this->MqttConfig::isEmpty()) {
        return false;
    }

    // Retry once in a while to avoid blocking serial console
    // Handling too big unsigned long
    if(lastConnectionRetry > ULONG_MAX - MQTT_CONNECT_RETRY_EVERY_MS) {
        lastConnectionRetry = 0;
    }
    if (lastConnectionRetry!=0 && lastConnectionRetry + MQTT_CONNECT_RETRY_EVERY_MS >= millis()) { // millis resets every 72 minutes
        // Too soon
        return false;
    }

    lastConnectionRetry = millis();

    char buf[128];
    String clientId = "YokisHack-";
    clientId += String(random(0xffff), HEX);

    sprintf(buf, "Connecting to MQTT %s:%hu with client ID=%s... ", getHost().c_str(), getPort(), clientId.c_str());
    LOG.print(buf);

    if (this->connect(clientId.c_str(), getUsername().c_str(), getPassword().c_str())) {
        LOG.println("connected");
        this->resubscribe();  // resubscribe to all configured topics
    } else {
        LOG.print("failed with state ");
        LOG.println(this->state());
    }

    return this->connected();
}

boolean Mqtt::loop() {
    if (this->MqttConfig::isEmpty()) {
        return false;
    }

    if(!this->connected()) {
        this->reconnect();
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
