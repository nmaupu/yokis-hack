#include "net/mqttHass.h"

MqttHass::MqttHass(WiFiClient& wifiClient, const char* host,
                   const uint16_t* port, const char* username,
                   const char* password)
    : Mqtt(wifiClient, host, port, username, password) {}

// Get JSON message to publish for HASS discovery
char* MqttHass::getMessageJson(const Device* device) {
    char bufMessage[512];
    char* ret;

    sprintf(bufMessage,
            "{"
            "\"name\":\"%s\","
            "\"cmd_t\":\"~cmnd/POWER\","
            "\"stat_t\":\"~tele/STATE\","
            "\"val_tpl\":\"{{value_json.POWER}}\","
            "\"pl_off\":\"OFF\","
            "\"pl_on\":\"ON\","
            "\"avty_t\":\"~tele/LWT\","
            "\"pl_avail\":\"Online\","
            "\"pl_not_avail\":\"Offline\","
            "\"uniq_id\":\"%s\","
            "\"device\":{\"identifiers\":[\"%s\"],\"cns\":[[\"ip\",\"%d.%d.%d.%"
            "d\"]],\"mf\":\"Yokis\"},"
            "\"~\":\"%s/\""
            "}",
            device->getDeviceName(), device->getDeviceName(),
            device->getDeviceName(), WiFi.localIP()[0], WiFi.localIP()[1],
            WiFi.localIP()[2], WiFi.localIP()[3], device->getDeviceName());

    ret = (char*)malloc(strlen(bufMessage) + 1);
    strcpy(ret, bufMessage);
    return ret;
}

char* MqttHass::getPublishTopic(const Device* device) {
    char bufTopic[128];
    char* ret;

    sprintf(bufTopic, "%s/light/%s/config", HASS_PREFIX,
            device->getDeviceName());

    ret = (char*)malloc(strlen(bufTopic) + 1);
    strcpy(ret, bufTopic);
    return ret;
}

// Publish device to MQTT for HASS discovery
bool MqttHass::publishDevice(const Device* device) {
    bool ret;
    char* topic = getPublishTopic(device);
    char* payload = getMessageJson(device);

    /*
    Serial.print(topic);
    Serial.print("=");
    Serial.println(payload);
    */

    ret = this->publish(topic, payload, true);
    if (ret) {
        notifyOnline(device);
    }

    free(topic);
    free(payload);
    return ret;
}

void MqttHass::notifyOnline(const Device* device) {
    char buf[64];

    sprintf(buf, "%s/tele/LWT", device->getDeviceName());
    publish(buf, "Online", true);

    notifyPower(device, "OFF"); // default status to OFF
}

void MqttHass::notifyPower(const Device* device, const char* status) {
    char buf[64];
    char bufPayload[64];

    sprintf(buf, "%s/tele/STATE", device->getDeviceName());
    sprintf(bufPayload, "{\"POWER\":\"%s\"}", status);
    publish(buf, bufPayload, false);

    sprintf(buf, "%s/stat/RESULT", device->getDeviceName());
    publish(buf, bufPayload, false);

    sprintf(buf, "%s/stat/POWER", device->getDeviceName());
    publish(buf, status, true);
}

// Subscribe device to be able to be controlled over MQTT
void MqttHass::subscribeDevice(const Device* device) {
    char buf[64];
    sprintf(buf, "%s/cmnd/POWER", device->getDeviceName());
    this->subscribe(buf);
}
