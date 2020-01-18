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

The following Yokis devices are supported:
- MTV500ER(P)
- MTR2000ER(P)

I never tested devices made for shutters (MVR500ER) because I don't have those...

## Download

No download available for now, you have to build yourself...
Coming soon though I hope.

## Building

- Install platform.io
- Compile with the following command (for ESP8266):
```
pio run
```

Resulting `firmwares` can be uploaded to any supported device.

Firware location depends on the device:
- Arduino Mega firmware location: `.pio/build/megaatmega2560/firmware.elf`
- ESP8266 firmware location: `.pio/build/d1_mini/firmware.bin`

To configure the ESP8266 build, use the following command:
```
MQTT_IP="<MQTT_IP>" \
MQTT_PORT="<MQTT_PORT>" \
MQTT_USERNAME="<MQTT_USERNAME>" \
MQTT_PASSWORD="<MQTT_PASSWORD>" \
WIFI_SSID="<SSID>" \
WIFI_PASSWORD="<WIFI_KEY>" \
  pio run -e d1_mini
```

First upload has to be done using an usb cable. However, all subsequent upgrades can be done using OTA as such (use `d1_mini_ota` instead of `d1_mini`):
```
MQTT_IP="<MQTT_IP>" \
MQTT_PORT="<MQTT_PORT>" \
MQTT_USERNAME="<MQTT_USERNAME>" \
MQTT_PASSWORD="<MQTT_PASSWORD>" \
WIFI_SSID="<SSID>" \
WIFI_PASSWORD="<WIFI_KEY>" \
  pio run -e d1_mini_ota --upload_port=<ip_address>
```

## Usage

### Hardware needed

- MCU (Arduino, ESP8266)
- NRF24L01+ (NRF24L01 should also work)

Remember that the NRF24 chip must be powered with **3.3V** !! It supports 5V on SPI though.

I used a small adapter to better regulate power to the NRF24L01+ easily found on *eBay*.

### Wiring and pinout

`CE`, `CS` and `IRQ` are specific to my implementation.
For `SPI` wiring, use your usual device ports.

|     | Arduino  | ESP8266  |
|-----|----------|----------|
| CE  | 7        | D2       |
| CS  | 8        | D8       |
| IRQ | 20       | D1       |

**Wiring `IRQ` pin is not optional !**

---

For example, here are the pinout of my testing MCUs:

|      | Wemos D1 mini | Arduino Mega |
|------|---------------|--------------|
| CLK  | D5            | 52           |
| MOSI | D7            | 51           |
| MISO | D6            | 50           |


### Firmware usage

#### Features

If you use Arduino, a very small set of features are available. Use an ESP8266 for all connectivity features (WiFi and MQTT).

The following features are available:
- Command line interface over serial (115200 bauds)
- Serial over Telnet connection
- OTA upgrades
- Home Assistant auto discovery (with prefix `/homeassistant` as describe on [Home Assistant documentation](https://www.home-assistant.io/docs/mqtt/discovery/))

From serial, one can use the following commands:

**Note:** To use telnet, just get the ip address of the device and use `telnet <ip>` to get the serial commands over Telnet.
**Note:** When you pair a device, its configuration is stored and can be use for various commands if you don't pass any `device_name` as parameter. If you pair a new device, it replaces the stored configuration.


| command   | paramters       | help                                                                                                |
|-----------|-----------------|-----------------------------------------------------------------------------------------------------|
| `help`    |                          | Display all commands available                                                                      |
| `debug`   |                          | Toggle debug mode                                                                                   |
| `raw`     |                          | Toggle raw output (output from pairing command will not be formatted) - deprecated                  |
| `config`  |                          | Display configuration flags state                                                                   |
| `poll`    |                          | Toggle the option to poll all the configured devices for their status and publish them over MQTT    |
| `pair`    |                          | Add a new device (emulate the pairing process) - it's like pressing a button's remote 5 times       |
| `copy`    | `[device_name]`          | Copy a device - send the payload corresponding the device to pair                                   |
| `scan`    | `[device_name]`          | scan for packet                                                                                     |
| `toggle`  | `[device_name]`          | Toggle the state of a device                                                                        |
| `on`      | `[device_name]`          | Switch on a device                                                                                  |
| `off`     | `[device_name]`          | Switch off a device                                                                                 |
| `press`   | `[device_name]`          | Emulate a button press (and hold) - this is mostly used for debugging                               |
| `pressFor`| `device_name duration`   | Emulate a button press and hold for a duration (dimmers only)                                       |
| `release` | `[device_name]`          | Emulate a button release - this is mostly used for debugging                                        |
| `dimmem`  | `[device_name]`          | Only for dimmers - Set to dimmer memory (1 button press)                                            |
| `dimmin`  | `[device_name]`          | Only for dimmers - Set the dimmer to minimum light (4 button presses)                               |
| `dimmax`  | `[device_name]`          | Only for dimmers - Set the dimmer to 100% light (2 button presses)                                  |
| `dimmid`  | `[device_name]`          | Only for dimmers - Set the dimmer to 50% light (3 button presses)                                   |
| `dimnil`  | `[device_name]`          | Only for dimmers - Set the dimmer *night light* mode (7 button presses)                             |
| `save`    |                          | Persist the current device to internal ESP memory (SPIFFS)                                          |
| `delete`  | `[device_name]`          | Delete one entry from the internal ESP memory (SPIFFS)                                              |
| `clear`   |                          | Clear the internal ESP memory and delete all stored devices(SPIFFS)                                 |
| `reload`  |                          | Reload the configuration from the internal ESP memory (SPIFFS)                                      |
| `dConfig` |                          | Display all loaded devices information                                                              |
| `dSpiffs` |                          | Display internal memory configuration file as is                                                    |

You need to use the serial for initial configuring and debugging.
After adding your devices, you don't need serial anymore.

#### Examples

##### Pair devices and save configuration

On the serial, use the `pair` command for each device you want to store and `save` it to config:
- `pair`
- click on the *connect* button on the back of the device

Instead of pressing the *connect* button at the back of the device which is sometimes not accessible. You can use an already paired remote:
- use the `pair` command like before
- press a remote's button for more than 3 seconds

##### Program a genuine Yokis remote

You (re)configure a genuine yokis remote, using the `copy` command like so:
- put the remote in pairing mode (5 short press on a button)
- use the `copy` command

### MQTT reference

Published topics:
- Auto discovery uses Home Assistant prefix: `homeassistant/`.
- Device's state is sent using the following topic pattern:
  - `<device_name>/tele/LWT` = `{Online,Offline}`
  - `<device_name>/tele/STATE` = `{"POWER":"On"}` or `{"POWER":"Off"}`
  - `<device_name>/tele/BRIGHTNESS` = `{"BRIGHTNESS":"value"}` where `value` is `0`, `1`, `2`, `3` or `4`. `3` and `4` are the same.

Subscribed topics:
- `<device_name>/cmnd/POWER`: `ON` or `OFF`
- `<device_name>/cmnd/BRIGHTNESS`: `0`, `1`, `2`, `3` or `4`
