#ifdef ESP8266
#include "net/mqttConfig.h"
#include "globals.h"

MqttConfig::MqttConfig() {
    port = MQTT_DEFAULT_PORT;
}

MqttConfig::MqttConfig(const MqttConfig& config) {
    setHost(config.host.c_str());
    setPort(config.port);
    setUsername(config.username.c_str());
    setPassword(config.password.c_str());
}

MqttConfig::MqttConfig(const char* host, uint16_t port, const char* username, const char* password) {
    // String memory management is handled by the = operator override
    setHost(host);
    setPort(port);
    setUsername(username);
    setPassword(password);
}

void MqttConfig::setHost(String host) { this->host = host; }

void MqttConfig::setPort(uint16_t){ this->port = port; }

void MqttConfig::setUsername(String username) { this->username = username; }

void MqttConfig::setPassword(String password) { this->password = password; }

String MqttConfig::getHost() { return host; }

uint16_t MqttConfig::getPort() { return port; }

String MqttConfig::getUsername() { return username; }

String MqttConfig::getPassword() { return password; }

bool MqttConfig::isEmpty() {
    return host.isEmpty();
}

void MqttConfig::printDebug(Print& p) {
    p.println("Host: " + getHost());

    char buf[6];
    sprintf(buf, "%d", getPort());
    p.print("Port: ");
    p.println(buf);

    p.println("Username: " + getUsername());
    p.println("Password: " + getPassword());
}

bool MqttConfig::saveToLittleFS() {
    bool ret = true;

    YokisLittleFS::init();

    File f = LittleFS.open(MQTT_CONFIG_FILE_NAME, "w");  // create or truncate file
    if (!f) {
        LOG.print(MQTT_CONFIG_FILE_NAME);
        LOG.println(" - file open failed for writing");
        return false;
    }

    char port_str[6]; // max length = 5 + \0 char
    sprintf(port_str, "%d", this->port);
    char* buf = (char*)malloc(
        sizeof(char) * (this->host.length() + strlen(port_str) +
                        this->username.length() + this->password.length() + 5));
    sprintf(buf, "%s|%s|%s|%s|", this->host.c_str(), port_str,
            this->username.c_str(), this->password.c_str());

    int bytesWritten = f.println(buf);
    if (bytesWritten <= 0) {
        LOG.print(MQTT_CONFIG_FILE_NAME);
        LOG.println(" - cannot write to file");
        ret = false;
    }

    free(buf);
    f.close();
    return ret;
}

// static
MqttConfig MqttConfig::loadFromLittleFS() {
    MqttConfig config;

    YokisLittleFS::init();
    File f = LittleFS.open(MQTT_CONFIG_FILE_NAME, "r");
    if (!f) {
        LOG.print(MQTT_CONFIG_FILE_NAME);
        LOG.println(" - File open failed for reading");
        return config;
    }

    if (f.available()) {
        config.setHost(f.readStringUntil('|'));
        config.setPort(atoi(f.readStringUntil('|').c_str()));
        config.setUsername(f.readStringUntil('|'));
        config.setPassword(f.readStringUntil('|'));
    }

    /*
    LOG.println("Config file debug");
    LOG.println("##########");
    f.seek(0);
    while(f.available()) {
        int c = f.read();
        LOG.print(c, HEX);
    }
    LOG.println();
    LOG.println("##########");
    */

    f.close();
    return config;
}

// static
bool MqttConfig::deleteConfigFromLittleFS() {
    YokisLittleFS::init();
    return LittleFS.remove(MQTT_CONFIG_FILE_NAME);
}
#endif // ESP8266
