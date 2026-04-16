#if WIFI_ENABLED && (defined(ESP8266) || defined(ESP32)) && WEBSERVER_ENABLED
#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include "net/wifi.h"

const char html_config_form[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Yokis-Hack</title>
        <style>
            body { font-family: Arial, sans-serif; max-width: 700px; margin: 0 auto; padding: 10px; }
            input[type=text], input[type=password], select {
                width: 100%%;
                padding: 12px 20px;
                margin: 8px 0;
                display: inline-block;
                border: 1px solid #ccc;
                border-radius: 4px;
                box-sizing: border-box;
            }
            input[type=submit] {
                width: 100%%;
                background-color: #4CAF50;
                color: white;
                padding: 14px 20px;
                margin: 8px 0;
                border: none;
                border-radius: 4px;
                cursor: pointer;
            }
            input[type=submit]:hover { background-color: #45a049; }
            .section {
                border-radius: 5px;
                background-color: #f2f2f2;
                padding: 20px;
                margin-bottom: 15px;
            }
            .alert {
                padding: 20px;
                background-color: #f44336;
                color: white;
                opacity: 0.83;
                transition: opacity 0.6s;
                margin-bottom: 15px;
            }
            .alert.success { background-color: #4CAF50; }
            .alert.info { background-color: #2196F3; }
            .alert.warning { background-color: #ff9800; }
            .closebtn {
                margin-left: 15px;
                color: white;
                font-weight: bold;
                float: right;
                font-size: 22px;
                line-height: 20px;
                cursor: pointer;
                transition: 0.3s;
            }
            .closebtn:hover { color: black; }
            .status-row { display: flex; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid #ddd; }
            .status-row:last-child { border-bottom: none; }
            .status-label { font-weight: bold; }
            .badge {
                display: inline-block;
                padding: 2px 10px;
                border-radius: 12px;
                color: white;
                font-size: 0.85em;
            }
            .badge-ok { background-color: #4CAF50; }
            .badge-err { background-color: #f44336; }
            .badge-warn { background-color: #ff9800; }
            table { width: 100%%; border-collapse: collapse; }
            th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }
            th { background-color: #4CAF50; color: white; }
            tr:hover { background-color: #e9e9e9; }
            .signal-bar { display: inline-block; width: 4px; margin-right: 1px; background: #ccc; vertical-align: bottom; }
            .signal-bar.active { background: #4CAF50; }
        </style>
        <script>
            function getMessageBox() {
                var urlParams = new URLSearchParams(window.location.search);
                var message = urlParams.get('message');
                if(message != null && message != "" && message != "null") {
                    document.getElementById("message").innerHTML = "<div class=\"alert success\"><span class=\"closebtn\" onclick=\"this.parentElement.style.display='none';\">&times;</span>" + message + "</div>";
                }
            }
            function signalBars(rssi) {
                var bars = 0;
                if (rssi > -50) bars = 4;
                else if (rssi > -60) bars = 3;
                else if (rssi > -70) bars = 2;
                else if (rssi > -80) bars = 1;
                var html = '';
                for (var i = 1; i <= 4; i++) {
                    html += '<span class="signal-bar' + (i <= bars ? ' active' : '') + '" style="height:' + (i*5) + 'px"></span>';
                }
                return html + ' ' + rssi + ' dBm';
            }
            function refreshStatus() {
                fetch('/api/status').then(function(r){return r.json()}).then(function(d) {
                    var ws = document.getElementById('wifi-status');
                    if (d.wifi.connected) {
                        ws.innerHTML = '<span class="badge badge-ok">Connected</span> to ' + d.wifi.ssid;
                        document.getElementById('wifi-ip').innerHTML = d.wifi.ip;
                        document.getElementById('wifi-rssi').innerHTML = signalBars(d.wifi.rssi);
                    } else {
                        ws.innerHTML = '<span class="badge badge-err">Disconnected</span>';
                        document.getElementById('wifi-ip').innerHTML = '-';
                        document.getElementById('wifi-rssi').innerHTML = '-';
                    }
                    var ms = document.getElementById('mqtt-status');
                    if (d.mqtt.connected) {
                        ms.innerHTML = '<span class="badge badge-ok">Connected</span> to ' + d.mqtt.host + ':' + d.mqtt.port;
                    } else if (d.mqtt.configured) {
                        ms.innerHTML = '<span class="badge badge-err">Disconnected</span>';
                    } else {
                        ms.innerHTML = '<span class="badge badge-warn">Not configured</span>';
                    }
                    document.getElementById('uptime').innerHTML = formatUptime(d.uptime);
                    document.getElementById('heap').innerHTML = d.heap + ' bytes';
                    var dt = document.getElementById('device-tbody');
                    if (d.devices.length === 0) {
                        dt.innerHTML = '<tr><td colspan="4" style="text-align:center">No devices paired</td></tr>';
                    } else {
                        var h = '';
                        for (var i = 0; i < d.devices.length; i++) {
                            var dev = d.devices[i];
                            var avail = dev.availability === 'Online'
                                ? '<span class="badge badge-ok">Online</span>'
                                : '<span class="badge badge-err">Offline</span>';
                            h += '<tr><td>' + dev.name + '</td><td>' + dev.mode + '</td><td>' + dev.status + '</td><td>' + avail + '</td></tr>';
                        }
                        dt.innerHTML = h;
                    }
                }).catch(function(){});
            }
            function formatUptime(s) {
                var d = Math.floor(s / 86400);
                var h = Math.floor((s %% 86400) / 3600);
                var m = Math.floor((s %% 3600) / 60);
                return (d > 0 ? d + 'd ' : '') + h + 'h ' + m + 'm';
            }
            window.onload = function() { getMessageBox(); refreshStatus(); setInterval(refreshStatus, 5000); };
        </script>
    </head>
    <body>
        <h1>Yokis-Hack</h1>

        <div id="message"></div>

        <div class="section">
            <h2>Status</h2>
            <div class="status-row"><span class="status-label">WiFi:</span><span id="wifi-status">...</span></div>
            <div class="status-row"><span class="status-label">IP:</span><span id="wifi-ip">...</span></div>
            <div class="status-row"><span class="status-label">Signal:</span><span id="wifi-rssi">...</span></div>
            <div class="status-row"><span class="status-label">MQTT:</span><span id="mqtt-status">...</span></div>
            <div class="status-row"><span class="status-label">Uptime:</span><span id="uptime">...</span></div>
            <div class="status-row"><span class="status-label">Free memory:</span><span id="heap">...</span></div>
        </div>

        <div class="section">
            <h2>Devices</h2>
            <table>
                <thead><tr><th>Name</th><th>Type</th><th>Status</th><th>Availability</th></tr></thead>
                <tbody id="device-tbody"><tr><td colspan="4" style="text-align:center">Loading...</td></tr></tbody>
            </table>
        </div>

        <div class="section">
        <form action="/save_config">

        <h2>WiFi</h2>
        <label for="wifi_ssid">SSID:</label>
        <input %WIFI_SECTION_ENABLED% type="text" id="wifi_ssid" name="wifi_ssid" value="%WIFI_SSID%" placeholder="Enter your WiFi SSID">

        <label for="wifi_password">Password:</label>
        <input %WIFI_SECTION_ENABLED% type="password" id="wifi_password" name="wifi_password" value="%WIFI_PASSWORD%" placeholder="Enter your WiFi password">

        <h2>MQTT</h2>
        <label for="mqtt_ip">IP:</label>
        <input %MQTT_SECTION_ENABLED% type="text" id="mqtt_ip" name="mqtt_ip" value="%MQTT_IP%" placeholder="192.168.0.1">

        <label for="mqtt_port">Port:</label>
        <input %MQTT_SECTION_ENABLED% type="text" id="mqtt_port" name="mqtt_port" value="%MQTT_PORT%" placeholder="1883">

        <label for="mqtt_username">Username:</label>
        <input %MQTT_SECTION_ENABLED% type="text" id="mqtt_username" name="mqtt_username" value="%MQTT_USERNAME%" placeholder="Username to connect to MQTT">

        <label for="mqtt_password">Password:</label>
        <input %MQTT_SECTION_ENABLED% type="password" id="mqtt_password" name="mqtt_password" value="%MQTT_PASSWORD%" placeholder="Password to connect to MQTT">

        <input type="submit" value="Save configuration">

        </form>
        </div>
    </body>
</html>
)rawliteral";

class WebServer : public AsyncWebServer {
   public:
     WebServer(uint16_t port);
     ~WebServer();
     static String processor(const String& var);
};

#endif  // __WEBSERVER_H__
#endif  // WIFI_ENABLED && (ESP8266 || ESP32) && WEBSERVER_ENABLED
