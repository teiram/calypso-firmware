#ifndef CRC_H
#define CRC_H

#include <inttypes.h>

class CRC {
public:
    static uint16_t crc16iv(const uint8_t *data, uint32_t len, uint16_t iv);
};

#endif //CRC_H