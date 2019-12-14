#include "utils.h"

#include <Arduino.h>

void printBinaryRepresentation(uint8_t n, bool leadingZero = false) {
    char buf[8 * sizeof(uint8_t) + 1];  // Assumes 8-bit chars plus zero byte.
    char* str = &buf[sizeof(buf) - 1];
    uint8_t nb = 0;

    *str = '\0';

    do {
        nb++;
        char c = n % 2;
        n /= 2;

        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while (n);

    while (nb++ < 8 && leadingZero) {
        *--str = '0';
    }

    Serial.print(str);
}
