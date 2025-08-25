#ifdef ESP8266

#include "serial/mySerial.h"

#define WEBSERIAL_BUFFER_LENGTH 1024

MySerial::MySerial(WebSerial* ws) : _ws(ws) {
  if(_ws) _ws->setBuffer(WEBSERIAL_BUFFER_LENGTH);
}

size_t MySerial::write(uint8_t c) {
  TelnetSpy::write(c);
  if(_ws) _ws->write(c);
  if(c == '\r' || c == '\n') {
    TelnetSpy::flush();
  }
  return 1;
}

#endif
