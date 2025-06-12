#ifndef PS2_KEYBOARD_H
#define PS2_KEYBOARD_H

#include "PS2Device.h"

namespace calypso {
    class PS2Keyboard: public PS2Device {
    public:
        static constexpr uint8_t BIT_SCROLL_LOCK = 1 << 0;
        static constexpr uint8_t BIT_NUMBER_LOCK = 1 << 1;
        static constexpr uint8_t BIT_SHIFT_LOCK = 1 << 2;

        PS2Keyboard(PIOContext &pio, CircularBuffer<uint8_t>& fifo, uint8_t clkGpio);
        bool init();
        bool setLeds(uint8_t value);
    private:
        static constexpr uint8_t PS2_KBD_RESET = 0xFF;
        static constexpr uint8_t PS2_KBD_LEDS = 0xED;
        static constexpr uint8_t PS2_KBD_ACK = 0xFA;
        static constexpr uint8_t PS2_KBD_REPEAT = 0xFE;
        static constexpr uint8_t PS2_SELF_TEST_PASSED = 0xAA;
    };
}
#endif //PS2_MOUSE_H