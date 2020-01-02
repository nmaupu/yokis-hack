#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <Arduino.h>

class Device {
   private:
    char* deviceName;
    uint8_t* hardwareAddress;
    uint8_t channel;
    uint8_t beginPacket;
    uint8_t endPacket;
#ifdef ESP8266
    static bool spiffsInitialized;
    // Init SPIFFS memory area
    static void spiffsInit();
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
    // Unallocate memory used
    ~Device();
    const char* getDeviceName() const;
    void setDeviceName(const char*);
    const uint8_t* getHardwareAddress() const;
    uint8_t getChannel() const;
    uint8_t getBeginPacket() const;
    uint8_t getEndPacket() const;
    void setBeginPacket(uint8_t);
    void setEndPacket(uint8_t);
    // set a 5 bytes hardware address
    void setHardwareAddress(const uint8_t*);
    // set a 5 bytes hardware address represented as 4 chars string
    // the resulting address will be "hw[0] hw[1] hw[0] hw[1] hw[1]"
    void setHardwareAddress(const char* hw);
    void setChannel(uint8_t);
    void printDebugInfo();
    void copy(const Device*);
    static Device* getFromList(Device**, size_t, const char*);
#ifdef ESP8266
    // Save device obj to SPIFFS
    bool saveToSpiffs();
    // Load a bunch of devices from SPIFFS to a previously allocated array of
    // pointers which size is passed as parameter
    static void loadFromSpiffs(Device**, const unsigned int);
    // Display config file from SPIFFS
    static void displayConfigFromSpiffs();
    // Clear config from SPIFFS
    static void clearConfigFromSpiffs();
#endif
};

#endif
