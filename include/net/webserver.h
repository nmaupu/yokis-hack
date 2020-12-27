#ifdef ESP8266
#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "net/wifi.h"

const char html_config_form[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Yokis-Hack configuration page</title>
        <style>
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

            input[type=submit]:hover {
                background-color: #45a049;
            }

            div {
                border-radius: 5px;
                background-color: #f2f2f2;
                padding: 20px;
            }

             /* The alert message box */
            .alert {
                padding: 20px;
                background-color: #f44336;
                color: white;
                opacity: 0.83;
                transition: opacity 0.6s;
                margin-bottom: 15px;
            }

            .alert.success {background-color: #4CAF50;}
            .alert.info {background-color: #2196F3;}
            .alert.warning {background-color: #ff9800;}

            /* The close button */
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

            /* When moving the mouse over the close button */
            .closebtn:hover {
                color: black;
            }
        </style>
        <script>
            function getMessageBox() {
                var urlParams = new URLSearchParams(window.location.search);
                var message = urlParams.get('message');
                if(message != null && message != "" && message != "null") {
                    document.getElementById("message").innerHTML = "<div class=\"alert success\"><span class=\"closebtn\" onclick=\"this.parentElement.style.display='none';\">&times;</span>" + message + "</div>";
                }
            }
        </script>
    </head>
    <body onload="getMessageBox()">
        <h1>Yokis-Hack configuration page</h1>

        <div id="message"></div>

        <div>
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
    String StatusMessage;

    WebServer(uint16_t port);
    ~WebServer();
    static String processor(const String& var);
};

#endif  // __WEBSERVER_H__
#endif  // ESP8266
