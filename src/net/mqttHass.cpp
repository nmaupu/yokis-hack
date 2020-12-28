#ifdef ESP8266
#include "net/mqttHass.h"
#include "RF/device.h"

MqttHass::MqttHass(WiFiClient& wifiClient) : Mqtt(wifiClient) {}

MqttHass::MqttHass(WiFiClient& wifiClient, const char* host,
                   const uint16_t port, const char* username,
                   const char* password)
    : Mqtt(wifiClient, host, port, username, password) {}

// Get JSON message to publish for HASS discovery
char* MqttHass::newMessageJson(const Device* device) {
    char bufMessage[1024];
    char* ret;

    if (device->getMode() == DIMMER) {
        sprintf(bufMessage,
                "{"
                "\"name\":\"%s dimmer\","
                "\"optimistic\":\"false\","  // if false, cannot know the status
                                             // of the device
                "\"on_command_type\":\"brightness\","  // only send brightness
                "\"bri_cmd_t\":\"~cmnd/BRIGHTNESS\","
                "\"bri_scl\":\"4\","  // 0=toggle, 1=min, 2=mid, 3/4=max
                "\"bri_stat_t\":\"~tele/BRIGHTNESS\","
                "\"bri_val_tpl\":\"{{value_json.BRIGHTNESS}}\","
                "\"cmd_t\":\"~cmnd/POWER\","
                "\"pl_off\":\"OFF\","
                "\"state_topic\":\"~tele/STATE\","
                "\"val_tpl\":\"{{value_json.POWER}}\","
                "\"avty_t\":\"~tele/LWT\","
                "\"pl_avail\":\"Online\","
                "\"pl_not_avail\":\"Offline\","
                "\"uniq_id\":\"esp-%s\","
                "\"device\":{"
                "\"name\":\"%s\","
                "\"identifiers\":[\"esp-%s\"],"
                "\"model\":\"MTV500ERX\","
                "\"mf\":\"Yokis\""
                "},"
                "\"~\":\"%s/\""
                "}",
                device->getName(), device->getName(), device->getName(),
                device->getName(), device->getName());
    } else {
        sprintf(bufMessage,
                "{"
                "\"name\":\"%s switch\","
                "\"optimistic\":\"false\","  // if false, cannot know the status
                                             // of the device
                "\"cmd_t\":\"~cmnd/POWER\","
                "\"state_topic\":\"~tele/STATE\","
                "\"val_tpl\":\"{{value_json.POWER}}\","
                "\"pl_off\":\"OFF\","
                "\"pl_on\":\"ON\","
                "\"avty_t\":\"~tele/LWT\","
                "\"pl_avail\":\"Online\","
                "\"pl_not_avail\":\"Offline\","
                "\"uniq_id\":\"esp-%s\","
                "\"device\":{"
                "\"name\":\"%s\","
                "\"identifiers\":[\"esp-%s\"],"
                "\"model\":\"MTR2000ERX\","
                "\"mf\":\"Yokis\""
                "},"
                "\"~\":\"%s/\""
                "}",
                device->getName(), device->getName(), device->getName(),
                device->getName(), device->getName());
    }

    ret = new char[strlen(bufMessage) + 1];
    strcpy(ret, bufMessage);
    return ret;
}

char* MqttHass::newPublishTopic(const Device* device) {
    char bufTopic[128];
    char* ret;

    sprintf(bufTopic, "%s/light/%s/config", HASS_PREFIX, device->getName());

    ret = new char[strlen(bufTopic) + 1];
    strcpy(ret, bufTopic);
    return ret;
}

// Publish device to MQTT for HASS discovery
bool MqttHass::publishDevice(const Device* device) {
    bool ret;
    char* topic = newPublishTopic(device);
    char* payload = newMessageJson(device);

    /*
    LOG.print(topic);
    LOG.print("=");
    LOG.println(payload);
    */

    ret = this->publish(topic, payload, true);
    if (ret) {
        notifyOnline(device);
    }

    delete[] topic;
    delete[] payload;
    return ret;
}

void MqttHass::notifyAvailability(const Device* device, const char* status) {
    char buf[64];

    sprintf(buf, "%s/tele/LWT", device->getName());
    publish(buf, status, true);
}

void MqttHass::notifyOnline(const Device* device) {
    notifyAvailability(device, "Online");
}

void MqttHass::notifyOffline(const Device* device) {
    notifyAvailability(device, "Offline");
}

void MqttHass::notifyPower(const Device* device) {
    notifyPower(device, device->getStatus());
}

void MqttHass::notifyPower(const Device* device, DeviceStatus ds) {
    char buf[64];
    char bufPayload[64];

    sprintf(buf, "%s/tele/STATE", device->getName());
    sprintf(bufPayload, "{\"POWER\":\"%s\"}", Device::getStatusAsString(ds));
    publish(buf, bufPayload, false);
}

void MqttHass::notifyBrightness(const Device* device) {
    char buf[64];
    char bufPayload[64];

    notifyPower(device, (device->getBrightness() == BRIGHTNESS_OFF ? OFF : ON));

    sprintf(buf, "%s/tele/BRIGHTNESS", device->getName());
    sprintf(bufPayload, "{\"BRIGHTNESS\":\"%d\"}", device->getBrightness());
    publish(buf, bufPayload, false);
}

// Subscribe device to be able to be controlled over MQTT
void MqttHass::subscribeDevice(const Device* device) {
    char buf[64];

    if (device->getMode() == ON_OFF || device->getMode() == NO_RCPT) {
        sprintf(buf, "%s/cmnd/POWER", device->getName());
        this->subscribe(buf);
    } else if (device->getMode() == DIMMER) {
        sprintf(buf, "%s/cmnd/POWER", device->getName());
        this->subscribe(buf);
        sprintf(buf, "%s/cmnd/BRIGHTNESS", device->getName());
        this->subscribe(buf);
    }
}
#endif
