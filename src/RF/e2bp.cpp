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
// Not reliable for DIMMER devices as status returned is the one BEFORE the
// command is executed
DeviceStatus E2bp::getLastKnownDeviceStatus() {
    if (secondPayloadStatus == UNDEFINED) {
        // We probably never received the second payload.
        // This happens in certain RF environments...
        return firstPayloadStatus;
    }

    // Second payload has the current device status
    // For ON_OFF devices, this is ok, each command is responding with the
    // status AFTER the command is executed For dimmers, the status is the one
    // BEFORE the command is executed !!
    return secondPayloadStatus;
}

bool E2bp::setDeviceStatus(DeviceStatus ds) {
    uint8_t buf[PAYLOAD_LENGTH];

    // For all devices, using the on/off payloads
    reset();
    setupRFModule();

    getPayload(buf, ds == ON ? PL_ON : PL_OFF);

    bool ret = false;
    unsigned long timeout = millis() + 1000;
    while (millis() <= timeout && !ret) ret = sendPayload(buf);

    if (ret || device->getMode() == NO_RCPT) {  // if NO_RCPT, ignore ret
        device->setStatus(ds);
        if (device->getMode() == DIMMER) {
            if (ds == ON)
                device->setBrightness(BRIGHTNESS_MAX);
            else
                device->setBrightness(BRIGHTNESS_OFF);
        }
    }

    return ret;
}

bool E2bp::on() { return setDeviceStatus(ON); }

bool E2bp::off() { return setDeviceStatus(OFF); }

// Toggle a device
// This function emulates a normal operated e2bp. But a better ON and OFF are
// more reliable and need less communication between devices To toggle, it's
// actually better to get status first and invert it.
bool E2bp::toggle() {
    unsigned long timeout = millis() + 1000;
    bool retPress = false, retRelease = false;

    reset();
    setupRFModule();
    while (millis() <= timeout) {
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
                break;  // assuming it has been toggled ok
            }
        }
    }

    // We don't test with retRelease because
    // we can have a device toggled without receiving a response...
    // if retPress is ok, we assume, device is toggled successfully
    // release set device status buf if we don't receive any response,
    // status won't be set...
    if (retPress && device->getMode() == ON_OFF) {
        // we know the status, set it
        device->setStatus(getLastKnownDeviceStatus());
    } else if (retPress && device->getMode() == DIMMER) {
        // dimmer replies with the current status BEFORE applying the command
        // so need to invert it
        device->setStatus(getLastKnownDeviceStatus());
        device->toggleStatus();
    }

    return retPress && retRelease;
}

// See Yokis MTV500ER manual for those configs
// Note: depending on configuration, 2 pulses can set to memory or 100%
// default is 100% for 2 pulses, that's what we will be using here...
bool E2bp::dimmerMem() { return dimmerSet(1); }
bool E2bp::dimmerMax() { return dimmerSet(2); }
bool E2bp::dimmerMid() { return dimmerSet(3); }
bool E2bp::dimmerMin() { return dimmerSet(4); }
bool E2bp::dimmerNiL() { return dimmerSet(7); }
bool E2bp::dimmerSet(const uint8_t number) {
    if (device->getMode() != DIMMER) {
        LOG.println("Not a dimmer device, ignoring.");
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

// Ask a device to give its status
DeviceStatus E2bp::pollForStatus() {
    // Using 0 for 'begin' packet seems to do the trick
    uint8_t buf[PAYLOAD_LENGTH];
    reset();
    setupRFModule();
    getPayload(buf, PL_STATUS);
    sendPayload(buf);
    return firstPayloadStatus;
}

// Get device mode from previous received data from the device
DeviceMode E2bp::getDeviceModeFromRecvData() {
    switch (answerBuf[0]) {
        case 0:
            return ON_OFF;
        case 1:
            return DIMMER;
        default:
            return NO_RCPT;
    }
}

bool E2bp::press() { return press(false); }

bool E2bp::press(bool dim) {
    bool ret = true;
    if (IS_DEBUG_ENABLED) LOG.println("Button pressing");
    uint8_t buf[PAYLOAD_LENGTH];

    firstPayloadStatus = UNDEFINED;
    if (dim)
        getPayload(buf, PL_DIM);
    else
        getPayload(buf, PL_BEGIN);
    ret = sendPayload(buf);

    if (IS_DEBUG_ENABLED) LOG.println("Button pressed");
    return ret;
}

bool E2bp::release() {
    bool ret = true;
    if (IS_DEBUG_ENABLED) LOG.println("Button releasing");
    uint8_t buf[PAYLOAD_LENGTH];

    getPayload(buf, PL_END);
    ret = sendPayload(buf);

    if (IS_DEBUG_ENABLED) LOG.println("Button released");
    if (ret && device->getMode() == ON_OFF) {
        device->setStatus(getLastKnownDeviceStatus());
    } else if (ret && device->getMode() == DIMMER) {
        device->setStatus(getLastKnownDeviceStatus());
        device->toggleStatus();
    }
    return ret;
}

// Press and hold a button
// Send begin payload and dimming payload while
// release() has not been called
bool E2bp::pressAndHoldFor(unsigned long duration) {
    // Only for dimmers and must be > 700ms
    if (device->getMode() != DIMMER || duration <= 700) return false;

    reset();
    setupRFModule();
    press();

    unsigned long timeout = millis() + duration;
    unsigned long startTime = millis();
    // Every second until timeout, send a dim command
    do {
        // send payload every 1 second
        if (millis() - startTime >= 1000) {
            startTime = millis();
            reset();
            setupRFModule();
            press(true);
        }
        yield();
        delayMicroseconds(1000);  // 1 ms delay
    } while (millis() <= timeout);

    bool ret = release();
    // no brightness level available but device is ON
    device->setBrightness(BRIGHTNESS_MAX);
    return ret;
}

// Fill a given buffer with the correct payload and return a pointer to it
uint8_t* E2bp::getPayload(uint8_t* buf, PayloadType type) {
    buf[0] = 0x00;
    buf[1] = 0x04;
    buf[2] = 0x00;
    buf[3] = 0x20;
    buf[4] = device->getHardwareAddress()[0];
    buf[5] = device->getHardwareAddress()[1];
    buf[6] = random(0, 0xff);
    buf[7] = 0x00;
    buf[8] = 0x00;

    switch (type) {
        case PL_BEGIN:
            buf[0] = YOKIS_CMD_BEGIN;
            break;
        case PL_END:
            buf[0] = YOKIS_CMD_END;
            break;
        case PL_ON:
            buf[0] = YOKIS_CMD_ON;
            break;
        case PL_OFF:
            buf[0] = YOKIS_CMD_OFF;
            break;
        case PL_STATUS:
            buf[0] = 0;
            buf[1] = 0;
            break;
        case PL_DIM:
            buf[0] = YOKIS_CMD_BEGIN;
            buf[7] = 0x02;
    }

    return buf;
}

bool E2bp::sendPayload(const uint8_t* payload) {
    if (IS_DEBUG_ENABLED) {
        LOG.print("Payload: ");
        for (uint8_t i = 0; i < 9; i++) {
            LOG.print(payload[i], HEX);
            LOG.print(" ");
        }
        LOG.println();
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

    yield();
    delayMicroseconds(4000);  // It's literally what I sniffed on the SPI
    flush_rx();               // done when calling begin() but anyway...
    if (IS_DEBUG_ENABLED) {
        printDetails();
    }
}

bool E2bp::runMainLoop() {
    unsigned long timeout = millis() + MAIN_LOOP_TIMEOUT_MILLIS;
    uint8_t nbLoops = 0;

    ce(LOW);
    // while not interrupted by RX, timeout or by device mode
    while (loopContinue && millis() <= timeout) {
        write_register(NRF_CONFIG, 0b00001110);  // PTX
        ce(HIGH);
        yield();
        delayMicroseconds(15);
        ce(LOW);
        yield();
        delayMicroseconds(685);

        write_register(NRF_CONFIG, 0b00001111);  // PRX
        ce(HIGH);
        write_register(NRF_STATUS, 0b01110000);  // Reset interrupts
        spiTrans(REUSE_TX_PL);
        yield();
        delayMicroseconds(1000);
        ce(LOW);

        if (device->getMode() == NO_RCPT && (nbLoops++) >= 30) {
            break;  // Stop sending, we are not gonna receive anything
        }
    }

    // We got here right after being interrupted
    if (available()) {
        read(answerBuf, 2);

        if (IS_DEBUG_ENABLED) {
            LOG.print("Received: ");
            printBinaryRepresentation(answerBuf[0], true);
            LOG.print(" ");
            printBinaryRepresentation(answerBuf[1], true);
            LOG.println();
        }

        if (firstPayloadStatus == UNDEFINED)
            firstPayloadStatus = answerBuf[1] == 1 ? ON : OFF;
        else
            secondPayloadStatus = answerBuf[1] == 1 ? ON : OFF;
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
        LOG.println("E2bp - TX OK");
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
        LOG.println("E2bp - RX READY");
    }
    // counter++;
    stopMainLoop();
}

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void E2bp::interruptTxFailed() {
    // Ignore
    LOG.println("TX sent failed");
}
