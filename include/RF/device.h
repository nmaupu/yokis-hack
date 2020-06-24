#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <Arduino.h>
#if defined(ESP8266)
#include <LittleFS.h>
#endif

// Depending on device, behavior can be different...
// Still to figure out what can configure that in scanned protocol though.
enum DeviceMode {
    ON_OFF = 0,  // Reception: OFF=00 00, ON=00 01
    DIMMER,   // Reception: 01 00 -> command not ok, 01 01 -> command ok (second
              // payload) - first payload responds with device status
    NO_RCPT,  // No reception, send blindly begin and end packets 30 times
    SHUTTER   // Shutter style devices (MVR500)
};

enum DeviceStatus { OFF = 0, ON, UNDEFINED, PAUSE_SHUTTER };

enum DeviceAvailability { OFFLINE = 0, ONLINE };

// Note: There is not 3 because in HASS, we use 5 values to
// have a slider with a 'middle' value.
// 3 is always considered as MAX value
enum DimmerBrightness {
    BRIGHTNESS_OFF = 0,
    BRIGHTNESS_MIN = 1,
    BRIGHTNESS_MID = 2,
    BRIGHTNESS_MAX = 4
};

class Device {
   private:
    char* name;
    uint8_t* hardwareAddress;
    uint8_t channel;
    uint8_t serial[2];
    uint8_t version[3];
    DeviceMode mode;
    DeviceStatus status;
    DeviceAvailability availability;
    DimmerBrightness brightness;  // only for dimmer device
    unsigned long lastUpdateMillis;
    bool hasToBePolledForStatus;
    uint8_t failedPolls;

#ifdef ESP8266
    static bool littleFSInitialized;
    // Init LittleFS memory area
    static void littleFSInit();
    // Search for a given device in config
    static int findInConfig(const char*);
    // Delete a line in config
    static void deleteLineInConfig(int line);
#endif

   public:
    // Create a device from another device
    Device(const Device* device);
    // Create a device with the given name
    Device(const char*);
    // Create a device with a name and parameters
    Device(const char*, const uint8_t*, uint8_t);
    // Create a device with a name and all parameters
    Device(const char*, const uint8_t*, uint8_t, const uint8_t*,
           const uint8_t*);
    // Unallocate memory used
    ~Device();

    // Getters
    const char* getName() const;
    const uint8_t* getHardwareAddress() const;
    uint8_t getChannel() const;
    const uint8_t* getVersion() const;
    const uint8_t* getSerial() const;
    const DeviceMode getMode() const;
    const DeviceStatus getStatus() const;
    const DimmerBrightness getBrightness() const;
    const DeviceAvailability getAvailability() const;
    // Last time device status was updated
    const unsigned long getLastUpdateMillis() const;
    bool needsPolling();
    static const char* getStatusAsString(DeviceStatus status);
    static const char* getModeAsString(DeviceMode mode);
    static const char* getAvailabilityAsString(DeviceAvailability);

    // Setters
    void setName(const char*);
    void setVersion(const uint8_t*);
    void setSerial(const uint8_t*);
    // set a 5 bytes hardware address
    void setHardwareAddress(const uint8_t*);
    // set a 5 bytes hardware address represented as 4 chars string
    // the resulting address will be "hw[0] hw[1] hw[0] hw[1] hw[1]"
    void setHardwareAddress(const char* hw);
    void setChannel(uint8_t);
    void setMode(DeviceMode);
    void setMode(const char*);
    void setStatus(DeviceStatus);
    void setBrightness(DimmerBrightness);
    void setAvailability(DeviceAvailability);
    void online();
    void offline();
    bool isOnline();
    bool isOffline();
    void pollMePlease();
    void pollingSuccess();
    uint8_t pollingFailed();
    uint8_t getFailedPollings();

    // misc
    void toSerial();
    void toggleStatus();
    void copy(const Device*);
    static Device* getFromList(Device**, size_t, const char*);
#ifdef ESP8266
    // Save device obj to LittleFS
    bool saveToLittleFS();
    // Store a raw device config to LittleFS (use to migrate from SPIFFS to LittleFS)
    static bool storeRawConfig(const char*);
    // Load a bunch of devices from LittleFS to a previously allocated array of
    // pointers which size is passed as parameter
    static void loadFromLittleFS(Device**, const unsigned int);
    // Display config file from LittleFS
    static void displayConfigFromLittleFS();
    // Clear config from LittleFS
    static void clearConfigFromLittleFS();
    // Delete a device from config
    static void deleteFromConfig(const char*);
#endif
};

#endif  // __DEVICE_H__
