#ifndef PS2_DEVICE_H
#define PS2_DEVICE_H

#include <inttypes.h>
#include "CircularBuffer.h"
#include "PIOContext.h"

extern PIOContext* pioContexts[];

extern "C" {
    inline static uint8_t decode(uint32_t value);
    static void irqCallback();
}

namespace calypso {
    class PS2Device {
    private:
        static bool PIO_INITIALIZED;
        uint8_t m_clkGpio;
        PIOContext &m_pio;
    protected:
        CircularBuffer<uint8_t> &m_fifo;
        bool waitDataAvailableWithTimeout(uint8_t size, uint16_t timeout);
    public:
        PS2Device(PIOContext &pio, CircularBuffer<uint8_t>& fifo, uint8_t clkGpio);
        virtual bool init();
        void disable();
        bool hasData();
        uint8_t available();
        uint8_t getData();
        void putData(uint8_t value);
        static inline uint8_t parity(uint8_t value);
    };
}
#endif //PS2_DEVICE_H