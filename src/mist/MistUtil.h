#ifndef MIST_UTIL_H
#define MIST_UTIL_H

#include <cinttypes>

namespace calypso {
    class MistUtil {
    public:
        static uint16_t collectBits(uint8_t const* data, uint16_t offset, uint8_t size, bool isSigned);
    };
}

#endif //MIST_UTIL_H