#include "net/mqttConfig.h"

MqttConfig::MqttConfig() {
    port = MQTT_DEFAULT_PORT;
}

MqttConfig::MqttConfig(const char* host, uint16_t port, const char* username, const char* password) {
    // String memory management is handled by the (operator =) func override
    setHost(host);
    setPort(port);
    setUsername(username);
    setPassword(password);
}

void MqttConfig::setHost(const char* host) { this->host = host; }

void MqttConfig::setPort(uint16_t){ this->port = port; }

void MqttConfig::setUsername(const char*) { this->username = username; }

void MqttConfig::setPassword(const char*) { this->password = password; }

// static
MqttConfig MqttConfig::loadConfig() {
    MqttConfig ret;
    return ret;
}

bool MqttConfig::saveConfig() {
    return true;
}

bool MqttConfig::empty() {
    return host.isEmpty();
}
