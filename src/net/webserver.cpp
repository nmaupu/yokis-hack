#ifdef ESP8266
#include "net/webserver.h"

WebServer::WebServer(uint16_t port) : AsyncWebServer(port) {
    this->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", html_config_form, processor);
    });

    this->on("/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Getting wifi configuration
        AsyncWebParameter* ssid = nullptr;
        AsyncWebParameter* password = nullptr;
        if(request->hasParam("wifi_ssid")) {
            ssid = request->getParam("wifi_ssid");
        }
        if(request->hasParam("wifi_password")) {
            password = request->getParam("wifi_password");
        }
        if(ssid != nullptr && password != nullptr) {
            setupWifi(ssid->value(), password->value());
        }

        request->redirect("/?message=Configuration saved successfully");
    });
}

WebServer::~WebServer() {}

// static
String WebServer::processor(const String& var) {
    if (var == "WIFI_SSID")
        return WiFi.SSID();
    else if (var == "WIFI_PASSWORD")
        return WiFi.psk();

    return String();
}

#endif // ESP8266
