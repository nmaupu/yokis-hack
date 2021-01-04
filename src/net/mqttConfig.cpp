#ifdef ESP8266
#include "net/mqttConfig.h"
#include "globals.h"

MqttConfig::MqttConfig() {
    strcpy(host, "");
    strcpy(username, "");
    strcpy(password, "");
}

MqttConfig::MqttConfig(const MqttConfig& config) {
    setHost(config.host);
    setPort(config.port);
    setUsername(config.username);
    setPassword(config.password);
}

MqttConfig::MqttConfig(const char* host, uint16_t port, const char* username, const char* password) {
    setHost(host);
    setPort(port);
    setUsername(username);
    setPassword(password);
}

MqttConfig::~MqttConfig() {
}

void MqttConfig::setHost(const char* h) {
    strncpy(host, h, MQTT_HOST_MAX_LENGTH);
}

void MqttConfig::setPort(uint16_t p) {
    this->port = p;
}

void MqttConfig::setUsername(const char* u) {
    strncpy(username, u, MQTT_USERNAME_MAX_LENGTH);
}

void MqttConfig::setPassword(const char* p) {
    strncpy(password, p, MQTT_PASSWORD_MAX_LENGTH);
}

char* MqttConfig::getHost() { return host; }

uint16_t MqttConfig::getPort() { return port; }

char* MqttConfig::getUsername() { return username; }

char* MqttConfig::getPassword() { return password; }

bool MqttConfig::isEmpty() {
    return host == nullptr || strlen(host) == 0;
}

void MqttConfig::printDebug(Print& p) {
    char buf[MQTT_HOST_MAX_LENGTH+11];

    sprintf(buf, "Host: %s", getHost());
    p.println(buf);
    sprintf(buf, "Port: %d", getPort());
    p.println(buf);
    sprintf(buf, "Username: %s", getUsername());
    p.println(buf);
    sprintf(buf, "Password: %s", getPassword());
    p.println(buf);
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

    String _host = String(this->host);
    String _username = String(this->username);
    String _password = String(this->password);

    char port_str[6]; // max length = 5 + \0 char
    sprintf(port_str, "%d", this->port);
    char* buf = (char*)malloc(
        sizeof(char) * (_host.length() + strlen(port_str) +
                        _username.length() + _password.length() + 5));
    sprintf(buf, "%s|%s|%s|%s|", _host.c_str(), port_str,
            _username.c_str(), _password.c_str());

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
        config.setHost(f.readStringUntil('|').c_str());
        config.setPort(atoi(f.readStringUntil('|').c_str()));
        config.setUsername(f.readStringUntil('|').c_str());
        config.setPassword(f.readStringUntil('|').c_str());
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
