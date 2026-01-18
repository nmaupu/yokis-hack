[![Build Status](https://travis-ci.org/nmaupu/yokis-hack.svg?branch=master)](https://travis-ci.org/nmaupu/yokis-hack)

# yokis-hack

Yokis-hack is a project around the reverse engineering of Yokis devices, a french company building stuff for home automation.
I started this project to better automate my own installation and to better understand the proprietary protocol driving my stuff :) !

Two devices interested me:
- MTV500ER (a dimmer)
- MTR2000ER (a switch)

Beware if you want to buy, `MTV500E` and `MTR2000E` are not driven using RF !

If you are interested, you can look at the [reverse engineering documentation](doc/intro.md).

## Hardware compatibility

This firmware can be compiled and installed on the following devices:
- Arduino
- ESP8266
- ESP32

The following Yokis devices are supported:
- MTV500ER(P)
- MTR2000ER(P)
- MTR2000MR(P)
- MVR500ER
- MTV300MRP


## Upgrading

### To v1.2

Configuration storage (ESP8266/ESP32 devices only) has changed (from SPIFFS to LittleFS) as of version *1.2*. As a result, before upgrading, copy your old configuration to be able to restore it afterward.

1. Carefully copy the result of the following command:
```
dSpiffs
```

2. Upgrade to v1.2. Your conf has now been erased.

3. Restore all your devices one at a time using the following command:
```
dRestore <line>
```

example:
```
dRestore lamp|6f4d|2d|00|00|490020|9f84|0000
```

### Upgrade general guidelines

- Make a backup of your configuration using the command `dConfigFS` (copy the content of the command's result somewhere safe)
- Upgrade using OTA or with `esptools.py`
- In case of issue with your config, restore it line by line using the command `dRestore`
- In case of weird bug (module stops functioning, weird stuff in the Serial, etc.), you need to completely erase the flash of the ESP8266/ESP32 and reburn the firmware.
  Erase can be done with:
  ```
  esptool.py -p /dev/<serial_port> erase_flash
  ```


## Usage

### Hardware needed

- MCU (Arduino, ESP8266, ESP32)
- NRF24L01+ (NRF24L01 should also work but remains untested)

Remember that the NRF24 chip must be powered with **3.3V** !! It supports 5V on SPI though.

I used a small adapter to better regulate power to the NRF24L01+ easily found on *eBay*.

### Wiring and pinout

`CE`, `CS` and `IRQ` are specific to my implementation.
For `SPI` wiring, use your usual device ports.

|     | Arduino  | ESP8266  | ESP32 |
|-----|----------|----------|-------|
| CE  | 7        | D2       | D4    |
| CS  | 8        | D8       | D5    |
| IRQ | 20       | D1       | D2    |

**Wiring `IRQ` pin is not optional !**

---

For example, here are the pinout of my testing MCUs:

|      | Wemos D1 mini | Arduino Mega | Elegoo ESP32 |
|------|---------------|--------------|--------------|
| CLK  | D5            | 52           | D18          |
| MOSI | D7            | 51           | D23          |
| MISO | D6            | 50           | D19          |

### Download and installation

See releases.

Tools needed: https://github.com/espressif/esptool

Can be installed with:

```
pip install esptool
```


Upload the binary file to your ESP8266 board using `esptools.py` like so:
```
esptool.py -p /dev/<serial_port> write_flash -fm dio 0x00000 /path/to/yokis-hack.bin
```

On ESP32, you need to write the `bootloader` and the `partitions` files as well:
```
esptool.py -p /dev/ttyUSB0 -b 460800 --before default-reset --after hard-reset --chip esp32 write-flash -fm dio --flash-size detect --flash-freq 40m 0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

`bootloader.bin`, `partitions.bin` and `firmware.bin` files can be found in the release page.


- Once uploaded and restarted, a new wifi network appears (named `YokisHack-XXXXXX`), connect to it.
- Open your browser to the following IP address: http://192.168.4.1
- Configure WiFi and optionally MQTT.

Once applied, you need to switch back to your WiFi. ESP gets its IP from DHCP so you have to figure out which ip it has now...

Then, to continue further (pairing devices, etc.), you can `telnet` to the ESP using its IP:
```
telnet <ip_address>
```


Note: It is also possible to configure everything using serial directly (with `minicom` for example).
The following commands can help:

```
> help
> wifiConfig
> wifiDiag
> mqttConfig
> mqttDiag
```


### Firmware usage

#### Features

If you use Arduino, a very small set of features are available. Use an ESP8266/ESP32 for all connectivity features (WiFi and MQTT).

The following features are available:
- Command line interface over serial (115200 bauds)
- Web UI to configure WiFi and MQTT
- Serial over Telnet connection
- OTA upgrades
- Home Assistant auto discovery (with prefix `/homeassistant` as describe on [Home Assistant documentation](https://www.home-assistant.io/docs/mqtt/discovery/))

From serial, one can use the following commands:

**Note:** To use telnet, just get the ip address of the device and use `telnet <ip>` to get the serial commands over Telnet.
**Note:** When you pair a device, its configuration is stored and can be use for various commands if you don't pass any `device_name` as parameter. If you pair a new device, it replaces the stored configuration.


| command            | paramters                | help                                                                                                |
|--------------------|--------------------------|-----------------------------------------------------------------------------------------------------|
| `help`             |                          | Display all commands available                                                                      |
| `debug`            |                          | Toggle debug mode                                                                                   |
| `raw`              |                          | Toggle raw output (output from pairing command will not be formatted) - deprecated                  |
| `poll`             |                          | Toggle the option to poll all the configured devices for their status and publish them over MQTT    |
| `config`           |                          | Display configuration flags state                                                                   |
| `pair`             |                          | Add a new device (emulate the pairing process) - it's like pressing a button's remote 5 times       |
| `toggle`           | `[device_name]`          | Toggle the state of a device                                                                        |
| `scan`             | `[device_name]`          | scan for packet                                                                                     |
| `copy`             | `[device_name]`          | Copy a device - send the payload corresponding the device to pair                                   |
| `dConfig`          |                          | Display all loaded devices information                                                              |
| `on`               | `[device_name]`          | Switch on a device                                                                                  |
| `off`              | `[device_name]`          | Switch off a device                                                                                 |
| `pause`            | `[device_name]`          | Pause the configured device (MVR500 only - shutter device)                                          |
| `press`            | `[device_name]`          | Emulate a button press (and hold) - this is mostly used for debugging                               |
| `pressFor`         | `device_name duration`   | Emulate a button press and hold for a duration (dimmers only)                                       |
| `release`          | `[device_name]`          | Emulate a button release - this is mostly used for debugging                                        |
| `status`           | `[device_name]`          | Get device status                                                                                   |
| `dimmem`           | `[device_name]`          | Only for dimmers - Set to dimmer memory (1 button press)                                            |
| `dimmin`           | `[device_name]`          | Only for dimmers - Set the dimmer to minimum light (4 button presses)                               |
| `dimmax`           | `[device_name]`          | Only for dimmers - Set the dimmer to 100% light (2 button presses)                                  |
| `dimmid`           | `[device_name]`          | Only for dimmers - Set the dimmer to 50% light (3 button presses)                                   |
| `dimnil`           | `[device_name]`          | Only for dimmers - Set the dimmer *night light* mode (7 button presses)                             |
| `save`             | `device_name`            | Persist the current device to internal ESP memory (LittleFS)                                        |
| `delete`           | `device_name`            | Delete one entry from the internal ESP memory (LittleFS)                                            |
| `clear`            |                          | Clear all config previously stored to LittleFS                                                      |
| `reload`           |                          | Reload config from LittleFS to memory                                                               |
| `dConfigFS`        |                          | display config previously stored in LittleFS                                                        |
| `dRestore`         | `config_line`            | restore a previously saved raw config line                                                          |
| `wifiConfig`       | `ssid password`          | Configure wifi with parameters: ssid psk (does not work for psk containing spaces)                  |
| `wifiDiag`         |                          | Display wifi configuration debug info                                                               |
| `wifiReset`        |                          | Reset wifi configuration and setup AP mode                                                          |
| `restart`          |                          | Restart the ESP board board                                                                           |
| `mqttConfig`       | `ip port user pass`      | Configure MQTT options (format: mqttConfig host port username password)                             |
| `mqttDiag`         |                          | Display current MQTT configuration                                                                  |
| `mqttConfigDelete` |                          | Delete current MQTT configuration                                                                   |

You need to use the serial or web ui for initial configuration.
Commands are only available through serial.

#### Examples

##### Pair devices and save configuration

On the serial, use the `pair` command for each device you want to store and `save` it to config:
- `pair`
- click on the *connect* button on the back of the device
- `save <device_name>`
- `reload`

Instead of pressing the *connect* button at the back of the device which is sometimes not accessible. You can use an already paired remote:
- use the `pair` command like before
- press a remote's button for more than 3 seconds

##### Program a genuine Yokis remote

To (re)configure a genuine yokis remote, use the *copy* command like so:
- put the remote in pairing mode (5 short presses on a button)
- use the `copy` command from the serial

### MQTT reference

Published topics:
- Auto discovery uses Home Assistant prefix: `homeassistant/`.
- Device's state is sent using the following topic patterns:
  - `<device_name>/tele/LWT` = `{Online,Offline}`
  - `<device_name>/tele/STATE` = `{"POWER":"On"}` or `{"POWER":"Off"}`
  - `<device_name>/tele/BRIGHTNESS` = `{"BRIGHTNESS":"value"}` where `value` is `0`, `1`, `2`, `3` or `4`. `3` and `4` are the same and set the device to *MAX* brightness.

Subscribed topics:
- `<device_name>/cmnd/POWER`: `ON` or `OFF`
- `<device_name>/cmnd/BRIGHTNESS`: `0`, `1`, `2`, `3` or `4`

## Development

- Install platform.io
- Compile with one of the following commands
``` sh
# d1_mini - ESP8266
pio run -e d1_mini -t upload

# ESP32
pio run -e esp32 -t upload
```

Or using docker:

``` sh
# d1_mini - ESP8266
make build

# ESP32
make build-esp32
```

Resulting `firmwares` can be uploaded to any supported device.

Firware location depends on the device:
- Arduino Mega firmware location: `.pio/build/megaatmega2560/firmware.elf`
- ESP8266 firmware location: `.pio/build/d1_mini/firmware.bin`
- ESP32:
  - firmware: `.pio/build/esp32/firmware.bin`
  - partitions: `.pio/build/esp32/partitions.bin`
  - bootloader: `.pio/build/esp32/bootloader.bin`

To configure the ESP build, use the following command:
```
MQTT_IP="<MQTT_IP>" \
MQTT_PORT="<MQTT_PORT>" \
MQTT_USERNAME="<MQTT_USERNAME>" \
MQTT_PASSWORD="<MQTT_PASSWORD>" \
WIFI_SSID="<SSID>" \
WIFI_PASSWORD="<WIFI_KEY>" \
  pio run -e d1_mini [-t upload]
```

First upload has to be done using an usb cable. However, all subsequent upgrades can be done using OTA as such (use `d1_mini_ota` instead of `d1_mini`):
```
MQTT_IP="<MQTT_IP>" \
MQTT_PORT="<MQTT_PORT>" \
MQTT_USERNAME="<MQTT_USERNAME>" \
MQTT_PASSWORD="<MQTT_PASSWORD>" \
WIFI_SSID="<SSID>" \
WIFI_PASSWORD="<WIFI_KEY>" \
  pio run -e d1_mini_ota --upload-port=<ip_address> -t upload
```

No OTA is being configured yet for ESP32.
