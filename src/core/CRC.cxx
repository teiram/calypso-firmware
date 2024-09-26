#include "CRC.h"

uint16_t CRC::crc16iv(const uint8_t *data, uint32_t len, uint16_t iv) {
    uint8_t x;
    uint16_t crc = iv;

    while (len--) {
        x = crc >> 8 ^ *data++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((uint16_t) (x << 12)) ^ ((uint16_t) (x << 5)) ^ ((uint16_t) x);
    }
    return crc;
}
