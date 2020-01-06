#include "RF/e2bp.h"
#include "globals.h"
#include "utils.h"

E2bp::E2bp(uint16_t cepin, uint16_t cspin) : RFConfigurator(cepin, cspin) {
    device = new Device("e2bp");
    this->firstPayloadStatus = UNDEFINED;
    this->secondPayloadStatus = UNDEFINED;
}

E2bp::~E2bp() { delete device; }

void E2bp::reset() {
    this->firstPayloadStatus = UNDEFINED;
    this->secondPayloadStatus = UNDEFINED;
    this->loopContinue = true;
}

void E2bp::setDevice(Device* device) { this->device = device; }

const Device* E2bp::getDevice() { return this->device; }

// Returns last known device status
// Not reliable for DIMMER devices
DeviceStatus E2bp::getLastKnownDeviceStatus() {
    if (secondPayloadStatus == UNDEFINED) {
        // We probably never received the second payload.
        // This happens in certain RF environments...
        return firstPayloadStatus;
    }

    // Second payload has the device status (for ON_OFF devices)
    return secondPayloadStatus;
}

bool E2bp::setDeviceStatus(DeviceStatus ds) {
    if (device->getMode() == NO_RCPT) {
        // set device status is not supported, toggle instead
        Serial.println(
            "on/off is not supported with this device, using toggle instead");
        return toggle();
    } else if (device->getMode() == DIMMER) {
        if (ds == ON) {
            // To be sure it's ON, set to mid (3 pulses)
            // HASS will set brightness to max when powered on though
            return dimmerMid();
        } else {
            // To be sure it's OFF, set to max and toggle
            dimmerMax();
            delay(800);
            bool ret = toggle();
            if (ret) device->setStatus(OFF);
            return ret;
        }
    }

    // ON_OFF device
    unsigned long timeout = millis() + 1000;
    bool retPress = false;
    bool retRelease = false;
    reset();
    setupRFModule();
    while (millis() <= timeout && secondPayloadStatus != ds) {
        retPress = press();
        if (retPress) retRelease = release();
    }

    // We don't test with retRelease because
    // we can have a device toggled without receiving a response...
    // if retPress is ok, we assume, device is toggled successfully
    // Even if status is set when calling release, force status here
    // just in case.
    if (retPress) device->setStatus(ds);
    return retPress && retRelease;
}

bool E2bp::on() { return setDeviceStatus(ON); }

bool E2bp::off() { return setDeviceStatus(OFF); }

bool E2bp::toggle() {
    unsigned long timeout = millis() + 1000;
    bool retPress = false, retRelease = false;

    reset();
    setupRFModule();
    while (millis() < timeout) {
        retPress = press();
        if (device->getMode() == DIMMER)
            delay(100);  // No need to wait for on/off devices

        if (retPress) {  // only do it if press has been ack (true)
            retRelease = release();

            if (device->getMode() == ON_OFF) {
                if (firstPayloadStatus != secondPayloadStatus &&
                    secondPayloadStatus != UNDEFINED)
                    break;  // Status changed successfully
            } else if (device->getMode() == DIMMER) {
                break;
            }
        }
    }

    // We don't test with retRelease because
    // we can have a device toggled without receiving a response...
    // if retPress is ok, we assume, device is toggled successfully
    // release set device status buf if we don't receive any response,
    // status won't be set... Redo it here.
    if (retPress && device->getMode() == ON_OFF) {
        // we know the status, set it
        device->setStatus(getLastKnownDeviceStatus());
    } else if (retPress && device->getMode() == DIMMER) {
        // Seems to be receiving 0 for ON and 1 for OFF
        // but not really sure about that :/
        device->setStatus(getLastKnownDeviceStatus());
        device->toggleStatus();
    }
    return retPress && retRelease;
}

// See Yokis MTV500ER manual for this configs
// Note: depending on configuration, 2 pulses can set to memory or 100%
// default is 100% for 2 pulses, that's what we will be using here...
bool E2bp::dimmerMem() { return dimmerSet(1); }
bool E2bp::dimmerMax() { return dimmerSet(2); }
bool E2bp::dimmerMid() { return dimmerSet(3); }
bool E2bp::dimmerMin() { return dimmerSet(4); }
bool E2bp::dimmerNiL() { return dimmerSet(7); }
bool E2bp::dimmerSet(const uint8_t number) {
    if (device->getMode() != DIMMER) {
        Serial.println("Not a dimmer device, ignoring.");
        return false;
    }

    bool ret = true;

    for (uint8_t i = 0; i < number && ret; i++) {
        ret = toggle();
        delay(10);
    }

    // Force status as we know it for sure !
    // 0 = no action
    // 1 = simple toggle
    // 2 = max (or memory)
    // 3 = mid
    // 4 = min
    if (number > 1 && ret)
        device->setStatus(ON);
    else if (number == 1 && ret)
        device->toggleStatus();

    if (ret) {
        switch (number) {
            case 0:
                device->setBrightness(BRIGHTNESS_OFF);
                break;
            case 2:
                device->setBrightness(BRIGHTNESS_MAX);
                break;
            case 3:
                device->setBrightness(BRIGHTNESS_MID);
                break;
            case 4:
                device->setBrightness(BRIGHTNESS_MIN);
                break;
        }
    }

    return ret;
}

bool E2bp::press() {
    bool ret = true;
    if (IS_DEBUG_ENABLED) Serial.println("Button pressing");
    uint8_t buf[PAYLOAD_LENGTH];

    firstPayloadStatus = UNDEFINED;
    getFirstPayload(buf);
    ret = sendPayload(buf);

    if (IS_DEBUG_ENABLED) Serial.println("Button pressed");
    return ret;
}

bool E2bp::release() {
    bool ret = true;
    if (IS_DEBUG_ENABLED) Serial.println("Button releasing");
    uint8_t buf[PAYLOAD_LENGTH];

    getSecondPayload(buf);
    ret = sendPayload(buf);

    if (IS_DEBUG_ENABLED) Serial.println("Button released");
    if (ret && device->getMode() == ON_OFF) {
        device->setStatus(getLastKnownDeviceStatus());
    } else if (ret && device->getMode() == DIMMER) {
        device->setStatus(getLastKnownDeviceStatus());
        device->toggleStatus();
    }
    return ret;
}

void E2bp::getFirstPayload(uint8_t* buf) {
    buf[0] = device->getBeginPacket();
    buf[1] = 0x04;
    buf[2] = 0x00;
    buf[3] = 0x20;
    buf[4] = device->getHardwareAddress()[0];
    buf[5] = device->getHardwareAddress()[1];
    // buf[4] = 0;
    // buf[5] = 0;
    buf[6] = random(0, 0xff);
    buf[7] = 0x00;
    buf[8] = 0x00;
}

void E2bp::getSecondPayload(uint8_t* buf) {
    buf[0] = device->getEndPacket();
    buf[1] = 0x04;
    buf[2] = 0x00;
    buf[3] = 0x20;
    buf[4] = device->getHardwareAddress()[0];
    buf[5] = device->getHardwareAddress()[1];
    // buf[4] = 0;
    // buf[5] = 0;
    buf[6] = random(0, 0xff);
    buf[7] = 0x00;
    buf[8] = 0x00;
}

bool E2bp::sendPayload(const uint8_t* payload) {
    if (IS_DEBUG_ENABLED) {
        Serial.print("Payload: ");
        for (uint8_t i = 0; i < 9; i++) {
            Serial.print(payload[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    setupPayload(payload);
    return runMainLoop();
}

void E2bp::setupRFModule() {
    begin();
    write_register(RF_CH, device->getChannel());
    // setChannel(device->getChannel());

    write_register(RF_SETUP, 0b00100011);
    // setDataRate(RF24_250KBPS);
    // setPALevel(RF24_PA_LOW);

    write_register(RX_ADDR_P0, device->getHardwareAddress(), 5);
    // setAddressWidth(5);
    // openReadingPipe(0, device->getHardwareAddress());

    write_register(TX_ADDR, device->getHardwareAddress(), 5);
    // openWritingPipe(device->getHardwareAddress());

    write_register(RX_PW_P0, 0x02);
    // setPayloadSize(0x02);

    write_register(EN_RXADDR, 1);  // openreadingpipe set this already
    write_register(EN_AA, 0);
    // setAutoAck(false);

    write_register(NRF_STATUS, 0b01110000);
    write_register(SETUP_RETR, 0);
    // setRetries(0, 0);

    write_register(NRF_CONFIG, 0b00001110);
    // openWritingPipe(device->getHardwareAddress());  // set to TX mode

    delay(4);    // It's literally what I sniffed on the SPI
    flush_rx();  // done when calling begin() but anyway...
    if (IS_DEBUG_ENABLED) {
        printDetails();
    }
}

bool E2bp::runMainLoop() {
    unsigned long timeout = millis() + MAIN_LOOP_TIMEOUT_MILLIS;
    uint8_t buf[2];
    uint8_t nbLoops = 0;

    ce(LOW);
    // while not interrupted by RX, timeout or by device mode
    while (loopContinue && millis() < timeout) {
        write_register(NRF_CONFIG, 0b00001110);  // PTX
        ce(HIGH);
        delayMicroseconds(15);
        ce(LOW);
        delayMicroseconds(685);

        write_register(NRF_CONFIG, 0b00001111);  // PRX
        ce(HIGH);
        write_register(NRF_STATUS, 0b01110000);  // Reset interrupts
        spiTrans(REUSE_TX_PL);
        delay(1);
        ce(LOW);

        nbLoops++;
        if (device->getMode() == NO_RCPT && nbLoops >= 30) {
            break;  // Stop sending, we are not gonna receive anything
        }
    }

    // We got here right after being interrupted
    if (available()) {
        read(buf, 2);

        if (IS_DEBUG_ENABLED) {
            Serial.print("Received: ");
            printBinaryRepresentation(buf[0], true);
            Serial.print(" ");
            printBinaryRepresentation(buf[1], true);
            Serial.println();
        }

        if (firstPayloadStatus == UNDEFINED)
            firstPayloadStatus = buf[1] == 1 ? ON : OFF;
        else
            secondPayloadStatus = buf[1] == 1 ? ON : OFF;
    }

    return !loopContinue;  // false if timeout occured
}

void E2bp::setupPayload(const uint8_t* payload) {
    // Prepare everything to send continuously the given payload
    loopContinue = true;
    setPayloadSize(PAYLOAD_LENGTH);
    flush_tx();
    write_payload(payload, PAYLOAD_LENGTH, W_TX_PAYLOAD);
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void E2bp::stopMainLoop() {
    loopContinue = false;
    // counter = 0;
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void E2bp::interruptTxOk() {
    if (IS_DEBUG_ENABLED) {
        Serial.println("E2bp - TX OK");
    }
    write_register(NRF_CONFIG, 0b00001111);  // PRX
    ce(HIGH);
    write_register(NRF_STATUS, 0b01110000);  // Reset interrupts
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void E2bp::interruptRxReady() {
    if (IS_DEBUG_ENABLED) {
        Serial.println("E2bp - RX READY");
    }
    // counter++;
    stopMainLoop();
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void E2bp::interruptTxFailed() {
    // Ignore
    Serial.println("TX sent failed");
}
