#ifndef __MY_SERIAL_H__
#define __MY_SERIAL_H__

#ifdef ESP8266
#include <TelnetSpy.h>
#include <MycilaWebSerial.h>

class MySerial : public TelnetSpy {
  private:
    WebSerial* _ws;

  public:
   explicit MySerial(WebSerial* ws = nullptr);
   size_t write(uint8_t c) override;
};

#endif

#endif //__MY_SERIAL_H__
