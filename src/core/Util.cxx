#include "Util.h"
#include <cstdio>
#include <cctype>

using namespace calypso;

void Util::hexdump(uint8_t *data, uint16_t size, uint16_t offset) {
    uint8_t i, b2c;
    uint16_t n = 0;
    uint8_t *ptr = data;

    while (size > 0) {
        printf("%04x: ", n + offset);
        uint8_t b2c = (size > 16) ? 16 : size;
        uint8_t i;
        for (i = 0; i < b2c ; i++) {
            printf("%02x ", ptr[i] & 0xff);
        }
        printf("  ");
        for (i = 0; i < (16 - b2c); i++) {
            printf("   ");
        }
        for (i = 0; i < b2c; i++) {
            printf("%c", isprint(ptr[i]) ? ptr[i] : '.');
        }
        printf("\n");
        ptr += b2c;
        size -= b2c;
        n += b2c;
    }
}
