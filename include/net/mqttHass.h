#ifdef ESP8266
#ifndef __MQTT_HASS_H__
#define __MQTT_HASS_H__

// Home assistant MQTT discovery
// See https://www.home-assistant.io/docs/mqtt/discovery

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "net/mqtt.h"
#include "RF/device.h"

#define HASS_PREFIX "homeassistant"

class MqttHass : public Mqtt {
    private:
    protected:
     char* getMessageJson(const Device*);
     char* getPublishTopic(const Device*);

    public:
     MqttHass(WiFiClient&, const char*, const uint16_t*, const char*, const char*);
     bool publishDevice(const Device*);
     void subscribeDevice(const Device*);
     void notifyOnline(const Device*);
     void notifyPower(const Device*, const char*);
};


#endif  // __MQTT_HASS_H__
#endif  // ESP8266
