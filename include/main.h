#ifndef __MAIN_H__
#define __MAIN_H__

#include <Arduino.h>

void setupForPairing();
void prepareForReading(uint8_t s);
bool receiveData(uint8_t s);
void showData(int num);
void giveMeInfo();

#endif  // __MAIN_H__