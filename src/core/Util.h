#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>

namespace calypso {
    class Util {
    public:
        static void hexdump(uint8_t *data, uint16_t size, uint16_t offset);
    };
}

#endif //UTIL_H
