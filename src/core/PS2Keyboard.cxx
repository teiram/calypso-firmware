#include "PS2Keyboard.h"
#include "calypso-debug.h"

using namespace calypso;

PS2Keyboard::PS2Keyboard(PIOContext& pio, CircularBuffer<uint8_t>& fifo, uint8_t clkGpio):
    PS2Device(pio, fifo, clkGpio) {}

bool PS2Keyboard::init() {
    if (PS2Device::init()) {
        m_fifo.clear();
        setLeds(PS2Keyboard::BIT_SCROLL_LOCK);
        return true;
    } else {
       PS2_DEBUG_LOG(L_ERROR, "Error initializing PS2Device\n");
       return false;
    }
}

bool PS2Keyboard::setLeds(uint8_t value) {
    PS2_DEBUG_LOG(L_DEBUG, "PS2Keyboard::setLeds(%02x)\n", value);
    putData(PS2_KBD_LEDS);
    bool timedOut = !waitDataAvailableWithTimeout(1, 200);
    if (!timedOut) {
        uint8_t r0 = getData();
        putData(value);
        timedOut = !waitDataAvailableWithTimeout(1, 200);
        if (!timedOut) {
            r0 = getData();
            PS2_DEBUG_LOG(L_TRACE, "Return code from setLeds: 0x%02x\n", r0);
            m_fifo.clear();
            return r0 == PS2_KBD_ACK;
        } else {
            PS2_DEBUG_LOG(L_ERROR, "Timeout setting kbd leds value\n");
        }
    } else {
        PS2_DEBUG_LOG(L_ERROR, "Timeout setting kbd leds command\n");
    }
    m_fifo.clear();
    return false;
}
