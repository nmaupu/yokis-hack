WIFI_SSID ?=
WIFI_PASSWORD ?=
PROG_VERSION ?=
MQTT_MAX_PACKET_SIZE ?= 1024

.PHONY: all
all: build

.PHONY: build
build:
	docker run --rm -it \
	  -v $(PWD):/opt/workspace \
	  -w /opt/workspace \
	  -e PLATFORMIO_CACHE_DIR=/opt/workspace/.pio-cache \
	  -e WIFI_SSID=$(WIFI_SSID) \
	  -e WIFI_PASSWORD=$(WIFI_PASSWORD) \
	  -e PROG_VERSION=$(PROG_VERSION) \
	  -e MQTT_MAX_PACKET_SIZE=$(MQTT_MAX_PACKET_SIZE) \
	  nmaupu/platformio-docker-build:latest pio run -e d1_mini
