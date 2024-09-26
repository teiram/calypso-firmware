#ifndef PS2_DEVICE_H
#define PS2_DEVICE_H

#include <inttypes.h>
#include "CircularBuffer.h"
#include "PIOProgram.h"

extern PIOProgram* pioPrograms[];

extern "C" {
    inline static uint8_t decode(uint32_t value);
    static void irqCallback();
}

namespace calypso {
    class PS2Device {
    private:
        uint8_t m_clkGpio;
        CircularBuffer<uint8_t> &m_fifo;
        PIOProgram &m_pio;
        inline uint8_t parity(uint8_t value);
    public:
        PS2Device(PIOProgram &pio, CircularBuffer<uint8_t>& fifo, uint8_t clkGpio);
        bool init();
        void disable();
        bool hasData();
        uint8_t getData();
        void putData(uint8_t value);
    };
}
#endif //PS2_DEVICE_H