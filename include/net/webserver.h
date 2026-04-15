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
            .btn-sm { padding: 4px 10px; border: none; border-radius: 3px; cursor: pointer; background: #2196F3; color: white; font-size: 0.85em; margin: 1px; }
            .btn-sm:hover { opacity: 0.85; }
            .btn-danger { background: #f44336; }
            .btn-warn { background: #ff9800; }
            .btn-bar { margin-top: 12px; display: flex; gap: 8px; flex-wrap: wrap; align-items: center; }
            .btn-bar button, .btn-bar label { margin: 0; }
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
                        dt.innerHTML = '<tr><td colspan="5" style="text-align:center">No devices paired</td></tr>';
                    } else {
                        var h = '';
                        for (var i = 0; i < d.devices.length; i++) {
                            var dev = d.devices[i];
                            var avail = dev.availability === 'Online'
                                ? '<span class="badge badge-ok">Online</span>'
                                : '<span class="badge badge-err">Offline</span>';
                            h += '<tr><td>' + dev.name + '</td><td>' + dev.mode + '</td><td>' + dev.status + '</td><td>' + avail + '</td>';
                            h += '<td><button class="btn-sm" onclick="renameDevice(\'' + dev.name + '\')">Rename</button> ';
                            h += '<button class="btn-sm btn-danger" onclick="deleteDevice(\'' + dev.name + '\')">Delete</button></td></tr>';
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
            function deleteDevice(name) {
                if (!confirm('Delete device "' + name + '"?')) return;
                fetch('/api/device/delete?name=' + encodeURIComponent(name))
                    .then(function(r){return r.json()})
                    .then(function(d){ if(d.ok) refreshStatus(); else alert(d.error); })
                    .catch(function(e){ alert('Error: ' + e); });
            }
            function renameDevice(oldName) {
                var newName = prompt('New name for "' + oldName + '":', oldName);
                if (!newName || newName === oldName) return;
                fetch('/api/device/rename?old=' + encodeURIComponent(oldName) + '&new=' + encodeURIComponent(newName))
                    .then(function(r){return r.json()})
                    .then(function(d){ if(d.ok) refreshStatus(); else alert(d.error); })
                    .catch(function(e){ alert('Error: ' + e); });
            }
            function exportConfig() {
                window.location.href = '/api/config/export';
            }
            function importConfig() {
                var input = document.getElementById('import-file');
                if (!input.files.length) { alert('Select a file first'); return; }
                var reader = new FileReader();
                reader.onload = function(e) {
                    if (!confirm('This will replace ALL device configuration. Continue?')) return;
                    fetch('/api/config/import', { method: 'POST', body: e.target.result })
                        .then(function(r){return r.json()})
                        .then(function(d){
                            if(d.ok) { alert('Configuration imported'); refreshStatus(); }
                            else alert(d.error);
                        })
                        .catch(function(e){ alert('Error: ' + e); });
                };
                reader.readAsText(input.files[0]);
            }
            function clearAllDevices() {
                if (!confirm('Delete ALL devices? This cannot be undone.')) return;
                fetch('/api/config/clear')
                    .then(function(r){return r.json()})
                    .then(function(d){ if(d.ok) refreshStatus(); else alert(d.error); })
                    .catch(function(e){ alert('Error: ' + e); });
            }
            var pairingPollTimer = null;
            function startPairing() {
                var name = document.getElementById('pair-name').value.trim();
                if (!name) { alert('Enter a name for the new device'); return; }
                var mode = document.getElementById('pair-mode').value;
                var url = '/api/pairing/start?name=' + encodeURIComponent(name);
                if (mode) url += '&mode=' + encodeURIComponent(mode);
                var box = document.getElementById('pair-status');
                box.innerHTML = '<span class="badge badge-warn">Starting...</span>';
                document.getElementById('btn-pair').disabled = true;
                fetch(url).then(function(r){return r.json()}).then(function(d) {
                    if (d.ok) {
                        box.innerHTML = '<span class="badge badge-warn">Waiting... Press the pairing button on your Yokis device</span>';
                        pairingPollTimer = setInterval(pollPairing, 1000);
                    } else {
                        box.innerHTML = '<span class="badge badge-err">' + d.error + '</span>';
                        document.getElementById('btn-pair').disabled = false;
                    }
                }).catch(function(e){
                    box.innerHTML = '<span class="badge badge-err">Error: ' + e + '</span>';
                    document.getElementById('btn-pair').disabled = false;
                });
            }
            function pollPairing() {
                fetch('/api/pairing/status').then(function(r){return r.json()}).then(function(d) {
                    var box = document.getElementById('pair-status');
                    if (d.state === 'success') {
                        box.innerHTML = '<span class="badge badge-ok">Paired!</span> ' + d.info;
                        clearInterval(pairingPollTimer);
                        document.getElementById('btn-pair').disabled = false;
                        document.getElementById('pair-name').value = '';
                        refreshStatus();
                    } else if (d.state === 'failed') {
                        box.innerHTML = '<span class="badge badge-err">Failed</span> ' + d.info;
                        clearInterval(pairingPollTimer);
                        document.getElementById('btn-pair').disabled = false;
                    } else if (d.state === 'in_progress' || d.state === 'requested') {
                        box.innerHTML = '<span class="badge badge-warn">Waiting... Press the pairing button on your Yokis device</span>';
                    }
                }).catch(function(){});
            }
            function uploadFirmware() {
                var input = document.getElementById('fw-file');
                if (!input.files.length) { alert('Select a .bin firmware file'); return; }
                if (!confirm('Upload new firmware? The device will reboot after update.')) return;
                var formData = new FormData();
                formData.append('firmware', input.files[0]);
                var box = document.getElementById('fw-status');
                box.innerHTML = '<span class="badge badge-warn">Uploading...</span>';
                document.getElementById('btn-fw').disabled = true;
                var xhr = new XMLHttpRequest();
                xhr.open('POST', '/api/firmware');
                xhr.upload.onprogress = function(e) {
                    if (e.lengthComputable) {
                        var pct = Math.round((e.loaded / e.total) * 100);
                        box.innerHTML = '<span class="badge badge-warn">Uploading... ' + pct + '%%</span>';
                    }
                };
                xhr.onload = function() {
                    try {
                        var d = JSON.parse(xhr.responseText);
                        if (d.ok) {
                            box.innerHTML = '<span class="badge badge-ok">Success! Rebooting...</span>';
                        } else {
                            box.innerHTML = '<span class="badge badge-err">' + (d.error||'Update failed') + '</span>';
                            document.getElementById('btn-fw').disabled = false;
                        }
                    } catch(e) {
                        box.innerHTML = '<span class="badge badge-ok">Uploaded, device rebooting...</span>';
                    }
                };
                xhr.onerror = function() {
                    box.innerHTML = '<span class="badge badge-err">Upload failed (network error)</span>';
                    document.getElementById('btn-fw').disabled = false;
                };
                xhr.send(formData);
            }
            function saveConfig() {
                var params = 'wifi_ssid=' + encodeURIComponent(document.getElementById('wifi_ssid').value)
                    + '&wifi_password=' + encodeURIComponent(document.getElementById('wifi_password').value)
                    + '&mqtt_ip=' + encodeURIComponent(document.getElementById('mqtt_ip').value)
                    + '&mqtt_port=' + encodeURIComponent(document.getElementById('mqtt_port').value)
                    + '&mqtt_username=' + encodeURIComponent(document.getElementById('mqtt_username').value)
                    + '&mqtt_password=' + encodeURIComponent(document.getElementById('mqtt_password').value);
                var box = document.getElementById('save-status');
                box.innerHTML = '<span class="badge badge-warn">Saving...</span>';
                document.getElementById('btn-save-config').disabled = true;
                fetch('/save_config?' + params)
                    .then(function(r){return r.json()})
                    .then(function(d){
                        if(d.ok) {
                            box.innerHTML = '<span class="badge badge-ok">' + d.message + '</span>';
                        } else {
                            box.innerHTML = '<span class="badge badge-err">' + (d.error||'Save failed') + '</span>';
                        }
                        document.getElementById('btn-save-config').disabled = false;
                    })
                    .catch(function(e){
                        box.innerHTML = '<span class="badge badge-err">Error: ' + e + '</span>';
                        document.getElementById('btn-save-config').disabled = false;
                    });
            }
            function fullBackup() {
                window.location.href = '/api/backup/export';
            }
            function fullRestore() {
                var input = document.getElementById('restore-file');
                if (!input.files.length) { alert('Select a backup file'); return; }
                if (!confirm('Restore full backup? This replaces devices AND MQTT configuration.')) return;
                var reader = new FileReader();
                reader.onload = function(e) {
                    fetch('/api/backup/import', { method: 'POST', body: e.target.result })
                        .then(function(r){return r.json()})
                        .then(function(d){
                            if (d.ok) { alert('Backup restored'); location.reload(); }
                            else alert(d.error);
                        })
                        .catch(function(e){ alert('Error: ' + e); });
                };
                reader.readAsText(input.files[0]);
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
                <thead><tr><th>Name</th><th>Type</th><th>Status</th><th>Availability</th><th>Actions</th></tr></thead>
                <tbody id="device-tbody"><tr><td colspan="5" style="text-align:center">Loading...</td></tr></tbody>
            </table>
            <div class="btn-bar">
                <button class="btn-sm" onclick="exportConfig()">Export config</button>
                <input type="file" id="import-file" accept=".txt,.conf,.bak" style="display:none" onchange="importConfig()">
                <button class="btn-sm btn-warn" onclick="document.getElementById('import-file').click()">Import config</button>
                <button class="btn-sm btn-danger" onclick="clearAllDevices()">Clear all</button>
            </div>
            <h3 style="margin-top:20px">Pair new device</h3>
            <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:end">
                <div style="flex:1;min-width:150px">
                    <label for="pair-name">Device name:</label>
                    <input type="text" id="pair-name" placeholder="e.g. salon" style="margin:4px 0">
                </div>
                <div style="min-width:120px">
                    <label for="pair-mode">Type:</label>
                    <select id="pair-mode" style="width:100%%;padding:12px;margin:4px 0;border:1px solid #ccc;border-radius:4px">
                        <option value="">Auto-detect</option>
                        <option value="ON_OFF">ON_OFF</option>
                        <option value="DIMMER">DIMMER</option>
                        <option value="SHUTTER">SHUTTER</option>
                    </select>
                </div>
                <div><button id="btn-pair" class="btn-sm" style="padding:12px 20px" onclick="startPairing()">Pair</button></div>
            </div>
            <div id="pair-status" style="margin-top:8px"></div>
        </div>

        <div class="section">
        <h2>WiFi</h2>
        <label for="wifi_ssid">SSID:</label>
        <input %WIFI_SECTION_ENABLED% type="text" id="wifi_ssid" value="%WIFI_SSID%" placeholder="Enter your WiFi SSID">

        <label for="wifi_password">Password:</label>
        <input %WIFI_SECTION_ENABLED% type="password" id="wifi_password" value="%WIFI_PASSWORD%" placeholder="Enter your WiFi password">

        <h2>MQTT</h2>
        <label for="mqtt_ip">IP:</label>
        <input %MQTT_SECTION_ENABLED% type="text" id="mqtt_ip" value="%MQTT_IP%" placeholder="192.168.0.1">

        <label for="mqtt_port">Port:</label>
        <input %MQTT_SECTION_ENABLED% type="text" id="mqtt_port" value="%MQTT_PORT%" placeholder="1883">

        <label for="mqtt_username">Username:</label>
        <input %MQTT_SECTION_ENABLED% type="text" id="mqtt_username" value="%MQTT_USERNAME%" placeholder="Username to connect to MQTT">

        <label for="mqtt_password">Password:</label>
        <input %MQTT_SECTION_ENABLED% type="password" id="mqtt_password" value="%MQTT_PASSWORD%" placeholder="Password to connect to MQTT">

        <button id="btn-save-config" style="width:100%%;background-color:#4CAF50;color:white;padding:14px 20px;margin:8px 0;border:none;border-radius:4px;cursor:pointer;font-size:1em" onclick="saveConfig()">Save configuration</button>
        <div id="save-status" style="margin-top:8px"></div>
        </div>

        <div class="section">
            <h2>Firmware Update</h2>
            <p style="margin:0 0 8px 0;color:#666;font-size:0.9em">Upload a .bin firmware file to update the device over the air.</p>
            <div style="display:flex;gap:8px;align-items:center;flex-wrap:wrap">
                <input type="file" id="fw-file" accept=".bin" style="flex:1;min-width:200px">
                <button id="btn-fw" class="btn-sm btn-warn" style="padding:12px 20px" onclick="uploadFirmware()">Upload &amp; Update</button>
            </div>
            <div id="fw-status" style="margin-top:8px"></div>
        </div>

        <div class="section">
            <h2>Backup &amp; Restore</h2>
            <p style="margin:0 0 8px 0;color:#666;font-size:0.9em">Full backup includes devices and MQTT configuration.</p>
            <div class="btn-bar">
                <button class="btn-sm" onclick="fullBackup()">Download backup</button>
                <input type="file" id="restore-file" accept=".json" style="display:none" onchange="fullRestore()">
                <button class="btn-sm btn-warn" onclick="document.getElementById('restore-file').click()">Restore backup</button>
            </div>
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
