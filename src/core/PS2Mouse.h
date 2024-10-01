#ifndef PS2_MOUSE_H
#define PS2_MOUSE_H

#include "PS2Device.h"

namespace calypso {
    class PS2Mouse: public PS2Device {
    public:
        PS2Mouse(PIOContext &pio, CircularBuffer<uint8_t>& fifo, uint8_t clkGpio);
        bool init();
    private:
        static constexpr uint8_t RESET = 0xff;
        static constexpr uint8_t SET_SAMPLE_RATE = 0xf3;
        static constexpr uint8_t ENABLE = 0xf4;
        static constexpr uint8_t ACK_OK = 0xfa;
        bool waitDataAvailableWithTimeout(uint8_t size, uint16_t timeout);
        bool enable();
        bool reset();
        bool setSampleRate(uint8_t sampleRate);
    };
}
#endif //PS2_MOUSE_H