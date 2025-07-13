#if WIFI_ENABLED && defined(ESP8266) && WEBSERVER_ENABLED
#include "net/webserver.h"
#include "globals.h"

WebServer::WebServer(uint16_t port) : AsyncWebServer(port) {
    this->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", html_config_form, processor);
    });

    this->on("/save_config", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Getting wifi configuration
        #ifndef WIFI_SSID
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
        #endif // WIFI_SSID

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
}

WebServer::~WebServer() {}

// static
String WebServer::processor(const String& var) {
    if (var == "WIFI_SECTION_ENABLED") {
        #ifdef WIFI_SSID
            return "disabled=\"\"";
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
