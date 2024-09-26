#include "db9.h"
#include "Joystick.h"
#include <inttypes.h>

#define DB9_UP          0x08
#define DB9_DOWN        0x04
#define DB9_LEFT        0x02
#define DB9_RIGHT       0x01
#define DB9_BTN1        0x10
#define DB9_BTN2        0x20

using namespace calypso;

extern Joystick joystick;

void InitDB9() {
    joystick.init();
}

char GetDB9(char index, unsigned char *joy_map) {
    static char data = 0;
    static int counter = 4;
    if (!index) {
        if (--counter == 0) {
            data = joystick.read();
            counter = 4;
        }
    }
    *joy_map = data;
    return 1;
}

static uint8_t legacy_mode = 0;
static void SetGpio(uint8_t usbjoy, uint8_t mask, uint8_t gpio) {}

void DB9SetLegacy(uint8_t on) {}

void DB9Update(uint8_t joy_num, uint8_t usbjoy) {}
