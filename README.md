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


## Download

No download available for now, you have to build yourself... Coming soon though.
