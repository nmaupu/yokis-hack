WIFI_ENABLED ?= 1
WEBSERVER_ENABLED ?= 1
MQTT_ENABLED ?= 1
WIFI_SSID ?=
WIFI_PASSWORD ?=
PROG_VERSION ?=
MQTT_MAX_PACKET_SIZE ?= 1024

# Force disabling mqtt and webserver if no wifi
ifeq ($(WIFI_ENABLED),0)
	MQTT_ENABLED=0
	WEBSERVER_ENABLED=0
endif

.PHONY: all
all: build

.PHONY: build
build:
	docker run --rm -it \
	  -v $(PWD):/opt/workspace \
	  -w /opt/workspace \
	  -e PLATFORMIO_CACHE_DIR=/opt/workspace/.pio-cache \
	  -e MQTT_ENABLED=$(MQTT_ENABLED) \
	  -e WEBSERVER_ENABLED=$(WEBSERVER_ENABLED) \
	  -e WIFI_ENABLED=$(WIFI_ENABLED) \
	  -e WIFI_SSID=$(WIFI_SSID) \
	  -e WIFI_PASSWORD=$(WIFI_PASSWORD) \
	  -e PROG_VERSION=$(PROG_VERSION) \
	  -e MQTT_MAX_PACKET_SIZE=$(MQTT_MAX_PACKET_SIZE) \
	  nmaupu/platformio-docker-build:latest pio run -e d1_mini

.PHONY: build-esp32
build-esp32:
	docker run --rm -it \
	  -v $(PWD):/opt/workspace \
	  -w /opt/workspace \
	  -e PLATFORMIO_CACHE_DIR=/opt/workspace/.pio-cache \
	  -e MQTT_ENABLED=$(MQTT_ENABLED) \
	  -e WEBSERVER_ENABLED=$(WEBSERVER_ENABLED) \
	  -e WIFI_ENABLED=$(WIFI_ENABLED) \
	  -e WIFI_SSID=$(WIFI_SSID) \
	  -e WIFI_PASSWORD=$(WIFI_PASSWORD) \
	  -e PROG_VERSION=$(PROG_VERSION) \
	  -e MQTT_MAX_PACKET_SIZE=$(MQTT_MAX_PACKET_SIZE) \
	  nmaupu/platformio-docker-build:latest pio run -e esp32

.PHONY: upload
upload:
	docker run --rm -it \
	  -v $(PWD):/opt/workspace \
	  -w /opt/workspace \
	  -e PLATFORMIO_CACHE_DIR=/opt/workspace/.pio-cache \
	  -e MQTT_ENABLED=$(MQTT_ENABLED) \
	  -e WEBSERVER_ENABLED=$(WEBSERVER_ENABLED) \
	  -e WIFI_ENABLED=$(WIFI_ENABLED) \
	  -e WIFI_SSID=$(WIFI_SSID) \
	  -e WIFI_PASSWORD=$(WIFI_PASSWORD) \
	  -e PROG_VERSION=$(PROG_VERSION) \
	  -e MQTT_MAX_PACKET_SIZE=$(MQTT_MAX_PACKET_SIZE) \
	  nmaupu/platformio-docker-build:latest pio run -e d1_mini_ota --upload-port=192.168.12.223 -t upload

release: build build-esp32
	mkdir release
	cp .pio/build/d1_mini/firmware.bin release/firmware-esp8266.bin
	cp .pio/build/esp32/firmware.bin release/firmware-esp32.bin
	cp .pio/build/esp32/bootloader.bin release/bootloader-esp32.bin
	cp .pio/build/esp32/partitions.bin release/partitions-esp32.bin
