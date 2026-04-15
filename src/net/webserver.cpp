#if WIFI_ENABLED && (defined(ESP8266) || defined(ESP32)) && WEBSERVER_ENABLED
#include "net/webserver.h"
#include "globals.h"
#include "RF/device.h"
#include "commands/callbacks.h"
#include "storage/yokisLittleFS.h"
#include <LittleFS.h>
#include <Updater.h>

// Deferred OTA reboot (defined in main.cpp)
extern volatile bool g_otaRebootPending;

// Deferred config save (defined in main.cpp)
extern volatile bool g_configSaveRequested;
extern char g_pendingWifiSsid[64];
extern char g_pendingWifiPassword[64];
extern char g_pendingMqttHost[64];
extern uint16_t g_pendingMqttPort;
extern char g_pendingMqttUsername[32];
extern char g_pendingMqttPassword[32];

// Pairing state machine (defined in main.cpp)
enum PairingState { PAIRING_IDLE, PAIRING_REQUESTED, PAIRING_IN_PROGRESS, PAIRING_SUCCESS, PAIRING_FAILED };
extern volatile PairingState g_pairingState;
extern String g_pairingSaveName;
extern String g_pairingMode;
extern String g_pairingInfo;

WebServer::WebServer(uint16_t port) : AsyncWebServer(port) {
    this->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", html_config_form, processor);
    });

    this->on("/save_config", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Don't do WiFi/MQTT work here — ESPAsyncWebServer runs in TCP callback
        // context. Blocking calls (WiFi.disconnect, delay, flash writes) cause
        // __yield panics and exception 9. Instead, copy params to buffers and
        // let the main loop handle it.

        // WiFi params
        g_pendingWifiSsid[0] = '\0';
        g_pendingWifiPassword[0] = '\0';

        #ifdef WIFI_SSID
            String wifiSsid = WIFI_SSID;
            bool allowWifiConfig = (wifiSsid.length() == 0);
        #else
            bool allowWifiConfig = true;
        #endif

        if (allowWifiConfig) {
            if (request->hasParam("wifi_ssid")) {
                strncpy(g_pendingWifiSsid, request->getParam("wifi_ssid")->value().c_str(), sizeof(g_pendingWifiSsid) - 1);
            }
            if (request->hasParam("wifi_password")) {
                strncpy(g_pendingWifiPassword, request->getParam("wifi_password")->value().c_str(), sizeof(g_pendingWifiPassword) - 1);
            }
        }

        // MQTT params
        g_pendingMqttHost[0] = '\0';
        g_pendingMqttPort = MQTT_DEFAULT_PORT;
        g_pendingMqttUsername[0] = '\0';
        g_pendingMqttPassword[0] = '\0';

        #if MQTT_ENABLED
        if (request->hasParam("mqtt_ip")) {
            strncpy(g_pendingMqttHost, request->getParam("mqtt_ip")->value().c_str(), sizeof(g_pendingMqttHost) - 1);
        }
        if (request->hasParam("mqtt_port")) {
            String v = request->getParam("mqtt_port")->value();
            if (v.length() > 0 && v.length() <= 5) {
                g_pendingMqttPort = (uint16_t)atol(v.c_str());
            }
        }
        if (request->hasParam("mqtt_username")) {
            strncpy(g_pendingMqttUsername, request->getParam("mqtt_username")->value().c_str(), sizeof(g_pendingMqttUsername) - 1);
        }
        if (request->hasParam("mqtt_password")) {
            strncpy(g_pendingMqttPassword, request->getParam("mqtt_password")->value().c_str(), sizeof(g_pendingMqttPassword) - 1);
        }
        #endif

        g_configSaveRequested = true;
        request->send(200, "application/json", "{\"ok\":true,\"message\":\"Configuration saved successfully\"}");
    });

    // JSON API for status dashboard
    this->on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Don't serve status during firmware update
        if (Update.isRunning()) {
            request->send(503, "application/json", "{\"error\":\"busy\"}");
            return;
        }

        // Pre-reserve to avoid heap fragmentation
        String json;
        json.reserve(512);
        json = "{";

        // WiFi status
        json += "\"wifi\":{\"connected\":";
        json += (WiFi.status() == WL_CONNECTED) ? "true" : "false";
        json += ",\"ssid\":\"";
        json += WiFi.SSID();
        json += "\",\"ip\":\"";
        json += WiFi.localIP().toString();
        json += "\",\"rssi\":";
        json += String(WiFi.RSSI());
        json += "}";

        // MQTT status
        #if MQTT_ENABLED
        json += ",\"mqtt\":{\"connected\":";
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
        json += ",\"uptime\":";
        json += String(millis() / 1000);
        json += ",\"heap\":";
        json += String(ESP.getFreeHeap());

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

    // Delete a device by name
    this->on("/api/device/delete", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("name")) {
            request->send(400, "application/json", "{\"error\":\"Missing name parameter\"}");
            return;
        }
        String name = request->getParam("name")->value();
        Device::deleteFromConfig(name.c_str());
        reloadConfig(NULL);
        request->send(200, "application/json", "{\"ok\":true}");
    });

    // Rename a device
    this->on("/api/device/rename", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("old") || !request->hasParam("new")) {
            request->send(400, "application/json", "{\"error\":\"Missing old or new parameter\"}");
            return;
        }
        String oldName = request->getParam("old")->value();
        String newName = request->getParam("new")->value();

        if (newName.length() == 0 || newName.indexOf('|') >= 0) {
            request->send(400, "application/json", "{\"error\":\"Invalid new name\"}");
            return;
        }

        // Find the device in memory, rename it, re-save
        Device* d = Device::getFromList(g_devices, MAX_YOKIS_DEVICES_NUM, oldName.c_str());
        if (d == NULL) {
            request->send(404, "application/json", "{\"error\":\"Device not found\"}");
            return;
        }
        // Delete old entry, rename, save new entry
        Device::deleteFromConfig(oldName.c_str());
        d->setName(newName.c_str());
        d->saveToLittleFS();
        reloadConfig(NULL);
        request->send(200, "application/json", "{\"ok\":true}");
    });

    // Export config as raw text file
    this->on("/api/config/export", HTTP_GET, [](AsyncWebServerRequest* request) {
        YokisLittleFS::init();
        File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "r");
        if (!f) {
            request->send(404, "application/json", "{\"error\":\"No config file\"}");
            return;
        }
        String content = f.readString();
        f.close();
        AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", content);
        response->addHeader("Content-Disposition", "attachment; filename=\"yokis-config.txt\"");
        request->send(response);
    });

    // Import config: replace whole config with uploaded text
    this->on("/api/config/import", HTTP_POST,
        // Response handler (called after body is received)
        [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", "{\"ok\":true}");
        },
        // File upload handler (not used)
        NULL,
        // Body handler
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                // First chunk: open file for writing (truncate)
                YokisLittleFS::init();
                File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "w");
                if (f) {
                    f.write(data, len);
                    f.close();
                }
            } else {
                // Subsequent chunks: append
                File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "a");
                if (f) {
                    f.write(data, len);
                    f.close();
                }
            }
            // On last chunk, reload config
            if (index + len == total) {
                reloadConfig(NULL);
            }
        }
    );

    // Start pairing - triggers pairing in main loop, browser polls for result
    this->on("/api/pairing/start", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (g_pairingState == PAIRING_IN_PROGRESS || g_pairingState == PAIRING_REQUESTED) {
            request->send(409, "application/json", "{\"error\":\"Pairing already in progress\"}");
            return;
        }
        // Optional: name to auto-save after pairing
        g_pairingSaveName = request->hasParam("name") ? request->getParam("name")->value() : "";
        g_pairingMode = request->hasParam("mode") ? request->getParam("mode")->value() : "";

        if (g_pairingSaveName.length() > 0 && g_pairingSaveName.indexOf('|') >= 0) {
            request->send(400, "application/json", "{\"error\":\"Invalid device name\"}");
            return;
        }

        g_pairingState = PAIRING_REQUESTED;
        request->send(200, "application/json", "{\"ok\":true,\"message\":\"Pairing started. Press the pairing button on your Yokis device within 30 seconds.\"}");
    });

    // Poll pairing status
    this->on("/api/pairing/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        String json = "{\"state\":\"";
        switch (g_pairingState) {
            case PAIRING_IDLE:        json += "idle"; break;
            case PAIRING_REQUESTED:   json += "requested"; break;
            case PAIRING_IN_PROGRESS: json += "in_progress"; break;
            case PAIRING_SUCCESS:     json += "success"; break;
            case PAIRING_FAILED:      json += "failed"; break;
        }
        json += "\",\"info\":\"" + g_pairingInfo + "\"}";
        // Reset state after client reads the result
        if (g_pairingState == PAIRING_SUCCESS || g_pairingState == PAIRING_FAILED) {
            g_pairingState = PAIRING_IDLE;
        }
        request->send(200, "application/json", json);
    });

    // Clear all devices
    this->on("/api/config/clear", HTTP_GET, [](AsyncWebServerRequest* request) {
        Device::clearConfigFromLittleFS();
        reloadConfig(NULL);
        request->send(200, "application/json", "{\"ok\":true}");
    });

    // Full backup: devices + MQTT config as JSON
    this->on("/api/backup/export", HTTP_GET, [](AsyncWebServerRequest* request) {
        YokisLittleFS::init();
        String json = "{";

        // Devices config
        json += "\"devices\":\"";
        File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "r");
        if (f) {
            while (f.available()) {
                char c = f.read();
                if (c == '\n') json += "\\n";
                else if (c == '\r') {} // skip
                else if (c == '"') json += "\\\"";
                else json += c;
            }
            f.close();
        }
        json += "\"";

        // MQTT config
        #if MQTT_ENABLED
        json += ",\"mqtt\":\"";
        File fm = LittleFS.open(MQTT_CONFIG_FILE_NAME, "r");
        if (fm) {
            while (fm.available()) {
                char c = fm.read();
                if (c == '\n') json += "\\n";
                else if (c == '\r') {}
                else if (c == '"') json += "\\\"";
                else json += c;
            }
            fm.close();
        }
        json += "\"";
        #endif

        json += "}";
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", json);
        response->addHeader("Content-Disposition", "attachment; filename=\"yokis-backup.json\"");
        request->send(response);
    });

    // Full restore: devices + MQTT config from JSON
    this->on("/api/backup/import", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", "{\"ok\":true}");
        },
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            // Accumulate body in a request-level temp string
            static String body;
            if (index == 0) body = "";
            body += String((char*)data).substring(0, len);

            if (index + len == total) {
                // Parse: find "devices":"..." and "mqtt":"..."
                YokisLittleFS::init();

                // Extract devices field
                int dStart = body.indexOf("\"devices\":\"");
                if (dStart >= 0) {
                    dStart += 11; // skip past "devices":"
                    int dEnd = body.indexOf("\"", dStart);
                    if (dEnd > dStart) {
                        String devConf = body.substring(dStart, dEnd);
                        devConf.replace("\\n", "\n");
                        devConf.replace("\\\"", "\"");
                        File f = LittleFS.open(LITTLEFS_CONFIG_FILENAME, "w");
                        if (f) { f.print(devConf); f.close(); }
                    }
                }

                // Extract mqtt field
                #if MQTT_ENABLED
                int mStart = body.indexOf("\"mqtt\":\"");
                if (mStart >= 0) {
                    mStart += 8;
                    int mEnd = body.indexOf("\"", mStart);
                    if (mEnd > mStart) {
                        String mqttConf = body.substring(mStart, mEnd);
                        mqttConf.replace("\\n", "\n");
                        mqttConf.replace("\\\"", "\"");
                        File fm = LittleFS.open(MQTT_CONFIG_FILE_NAME, "w");
                        if (fm) { fm.print(mqttConf); fm.close(); }
                    }
                }
                #endif

                reloadConfig(NULL);
                body = "";
            }
        }
    );

    // OTA firmware upload via web
    this->on("/api/firmware", HTTP_POST,
        // Response handler (called after upload completes)
        [](AsyncWebServerRequest* request) {
            bool success = !Update.hasError();
            AsyncWebServerResponse* response = request->beginResponse(
                200, "application/json",
                success ? "{\"ok\":true,\"message\":\"Firmware updated, rebooting...\"}"
                        : "{\"ok\":false,\"error\":\"Firmware update failed\"}");
            response->addHeader("Connection", "close");
            request->send(response);
            // Don't call delay() or ESP.restart() here — this runs in async
            // TCP context where yield/delay causes __yield panic.
            // Instead, schedule restart from the main loop.
            if (success) {
                g_otaRebootPending = true;
            }
        },
        // File upload handler (receives firmware chunks)
        [](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                LOG.println("OTA web upload started: " + filename);
                size_t maxSize = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                Update.runAsync(true);  // Don't call yield() during flash writes (async context safe)
                if (!Update.begin(maxSize)) {
                    LOG.println("OTA begin failed");
                    return;
                }
            }
            if (Update.isRunning()) {
                if (Update.write(data, len) != len) {
                    LOG.println("OTA write failed");
                }
            }
            if (final) {
                if (Update.end(true)) {
                    LOG.println("OTA web upload complete");
                } else {
                    LOG.println("OTA web upload failed");
                }
            }
        }
    );

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
        #if !MQTT_ENABLED
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
