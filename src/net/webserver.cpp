#if WIFI_ENABLED && (defined(ESP8266) || defined(ESP32)) && WEBSERVER_ENABLED
#include "net/webserver.h"
#include "globals.h"
#include "RF/device.h"

WebServer::WebServer(uint16_t port) : AsyncWebServer(port) {
    this->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", html_config_form, processor);
    });

    this->on("/save_config", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Getting wifi configuration
        // Allow saving if WIFI_SSID is not defined, or if it's defined but empty
        #ifdef WIFI_SSID
            String wifiSsid = WIFI_SSID;
            bool allowWifiConfig = (wifiSsid.length() == 0);
        #else
            bool allowWifiConfig = true;
        #endif // WIFI_SSID
        
        if(allowWifiConfig) {
            const AsyncWebParameter* ssid = nullptr;
            const AsyncWebParameter* password = nullptr;
            if(request->hasParam("wifi_ssid")) {
                ssid = request->getParam("wifi_ssid");
            }
            if(request->hasParam("wifi_password")) {
                password = request->getParam("wifi_password");
            }
            if(ssid != nullptr && password != nullptr) {
                setupWifi(ssid->value(), password->value());
            }
        }

        #if MQTT_ENABLED && !defined(MQTT_IP)
        // Getting MQTT configuration
        const AsyncWebParameter* mqttParam = nullptr;
        bool mqttChange = false;
        String c_host;
        uint16_t c_port = MQTT_DEFAULT_PORT;
        String c_username;
        String c_password;

        if (request->hasParam("mqtt_ip")) {
            mqttParam = request->getParam("mqtt_ip");
            if (mqttParam != nullptr) {
                c_host = mqttParam->value();
                mqttChange = true;
            }
        }
        if (request->hasParam("mqtt_port")) {
            mqttParam = request->getParam("mqtt_port");
            if (mqttParam != nullptr) {
                if (mqttParam->value().length() != 0 && mqttParam->value().length() <= 5) {
                    c_port = (uint16_t)atol(mqttParam->value().c_str());
                    mqttChange = true;
                }
            }
        }
        if (request->hasParam("mqtt_username")) {
            mqttParam = request->getParam("mqtt_username");
            if (mqttParam != nullptr) {
                c_username = mqttParam->value();
                mqttChange = true;
            }
        }
        if (request->hasParam("mqtt_password")) {
            mqttParam = request->getParam("mqtt_password");
            if (mqttParam != nullptr) {
                c_password = mqttParam->value();
                mqttChange = true;
            }
        }

        if(mqttChange) {
            // set new config, persist and reconnect
            g_mqtt->setDiscoveryDone(false);
            g_mqtt->setConnectionInfo(c_host.c_str(), c_port, c_username.c_str(), c_password.c_str());
        }

        #endif  // #if MQTT_ENABLED && !defined(MQTT_IP)

        request->redirect("/?message=Configuration saved successfully");
    });

    // JSON API for status dashboard
    this->on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        String json = "{";

        // WiFi status
        json += "\"wifi\":{";
        json += "\"connected\":";
        json += (WiFi.status() == WL_CONNECTED) ? "true" : "false";
        json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
        json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
        json += ",\"rssi\":" + String(WiFi.RSSI());
        json += "}";

        // MQTT status
        #if MQTT_ENABLED
        json += ",\"mqtt\":{";
        json += "\"connected\":";
        json += g_mqtt->connected() ? "true" : "false";
        json += ",\"configured\":";
        json += g_mqtt->MqttConfig::isEmpty() ? "false" : "true";
        json += ",\"host\":\"";
        json += g_mqtt->getHost();
        json += "\",\"port\":";
        json += String(g_mqtt->getPort());
        json += "}";
        #else
        json += ",\"mqtt\":{\"connected\":false,\"configured\":false,\"host\":\"\",\"port\":0}";
        #endif

        // Uptime & heap
        json += ",\"uptime\":" + String(millis() / 1000);
        #if defined(ESP8266)
        json += ",\"heap\":" + String(ESP.getFreeHeap());
        #elif defined(ESP32)
        json += ",\"heap\":" + String(ESP.getFreeHeap());
        #endif

        // Devices
        json += ",\"devices\":[";
        for (uint8_t i = 0; i < g_nb_devices; i++) {
            if (g_devices[i] != NULL) {
                if (i > 0) json += ",";
                json += "{\"name\":\"";
                json += g_devices[i]->getName();
                json += "\",\"mode\":\"";
                json += Device::getModeAsString(g_devices[i]->getMode());
                json += "\",\"status\":\"";
                json += Device::getStatusAsString(g_devices[i]->getStatus());
                json += "\",\"availability\":\"";
                json += Device::getAvailabilityAsString(g_devices[i]->getAvailability());
                json += "\"}";
            }
        }
        json += "]}";

        request->send(200, "application/json", json);
    });

    // Captive portal: redirect all unknown requests to the config page
    this->onNotFound([](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
}

WebServer::~WebServer() {}

// static
String WebServer::processor(const String& var) {
    if (var == "WIFI_SECTION_ENABLED") {
        #ifdef WIFI_SSID
            // Check at runtime if WIFI_SSID is non-empty
            String wifiSsid = WIFI_SSID;
            if (wifiSsid.length() > 0) {
                return "disabled=\"\"";
            } else {
                return "";
            }
        #else
            return "";
        #endif
    }

    if (var == "MQTT_SECTION_ENABLED") {
        #if !MQTT_ENABLED || defined(MQTT_IP)
            return "disabled=\"\"";
        #elif !MQTT_ENABLED
            return "disabled=\"\"";
        #else
            return "";
        #endif
    }

    if (var == "WIFI_SSID")
        return WiFi.SSID();

    if (var == "WIFI_PASSWORD")
        return WiFi.psk();

    #if MQTT_ENABLED
    if (var == "MQTT_IP")
        return g_mqtt->getHost();

    if (var == "MQTT_PORT")
        return String(g_mqtt->getPort(), 10);

    if (var == "MQTT_USERNAME")
        return g_mqtt->getUsername();

    if (var == "MQTT_PASSWORD")
        return g_mqtt->getPassword();
    #endif

    return String();
}

#endif  // WIFI_ENABLED && ESP8266 && WEBSERVER_ENABLED
