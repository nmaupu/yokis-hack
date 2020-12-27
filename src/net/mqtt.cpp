#ifdef ESP8266
#include "net/mqtt.h"
#include <Arduino.h>
#include "globals.h"

Mqtt::Mqtt(WiFiClient& wifiClient) : PubSubClient(wifiClient), MqttConfig() {
    // Init subscriptions to NULL
    subscribedTopicIdx = 0;
    for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
        subscribedTopics[i] = NULL;
    }
}

Mqtt::Mqtt(WiFiClient& wifiClient, MqttConfig& mqttConfig)
    : PubSubClient(wifiClient), MqttConfig(mqttConfig) {
    this->setCallback(Mqtt::callback);

    // Init subscriptions to NULL
    subscribedTopicIdx = 0;
    for (uint8_t i = 0; i < MQTT_MAX_NUM_OF_YOKIS_DEVICES; i++) {
        subscribedTopics[i] = NULL;
    }
}

// Set mqtt connection info, and optionally save config to LittleFS (default to
// true)
void Mqtt::setConnectionInfo(MqttConfig& config, bool saveConfig) {
    setConnectionInfo(config.getHost(), config.getPort(), config.getUsername(), config.getPassword(), saveConfig);
}

// Set mqtt connection info, and optionally save config to LittleFS (default to
// true)
void Mqtt::setConnectionInfo(const char* host, uint16_t port, const char* username, const char* password, bool saveConfig) {
    this->setHost(host);
    this->setPort(port);
    this->setUsername(username);
    this->setPassword(password);
    if(saveConfig) {
        this->saveToLittleFS();
    }
    //this->MqttConfig::printDebug(LOG);

    if (this->connected()) {
        this->disconnect();
    }

    this->setServer(getHost(), getPort());
    this->reconnect(true);
}

boolean Mqtt::subscribe(const char* topic) {
    LOG.print("Subscribing to topic: ");
    LOG.println(topic);
    if (subscribedTopicIdx >= MQTT_MAX_NUM_OF_YOKIS_DEVICES) return false;

    subscribedTopics[subscribedTopicIdx] = (char*)malloc(strlen(topic)+1);
    strlcpy(subscribedTopics[subscribedTopicIdx], topic, strlen(topic)+1);
    subscribedTopicIdx++;

    return PubSubClient::subscribe(topic);
}

void Mqtt::resubscribe() {
    LOG.print("Resubscribing, #topics=");
    LOG.println(subscribedTopicIdx);
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

// force=false by default
bool Mqtt::reconnect(bool force) {
    // No configuration available
    if (this->MqttConfig::isEmpty()) {
        return false;
    }

    // Retry once in a while to avoid blocking serial console
    // Handling too big unsigned long
    if(!force) {
        if(lastConnectionRetry > ULONG_MAX - MQTT_CONNECT_RETRY_EVERY_MS) {
            lastConnectionRetry = 0;
        }
        if (lastConnectionRetry!=0 && lastConnectionRetry + MQTT_CONNECT_RETRY_EVERY_MS >= millis()) { // millis resets every 72 minutes
            // Too soon
            return false;
        }
    }

    lastConnectionRetry = millis();

    char buf[128];
    String clientId = "YokisHack-";
    clientId += String(random(0xffff), HEX);

    sprintf(buf, "Connecting to MQTT %s:%hu with client ID=%s... ", getHost(), getPort(), clientId.c_str());
    LOG.print(buf);

    if (this->connect(clientId.c_str(), getUsername(), getPassword())) {
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
