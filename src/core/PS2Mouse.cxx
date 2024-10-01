#include "PS2Mouse.h"
#include "calypso-debug.h"
#include "pico/time.h"

using namespace calypso;

PS2Mouse::PS2Mouse(PIOContext& pio, CircularBuffer<uint8_t>& fifo, uint8_t clkGpio):
    PS2Device(pio, fifo, clkGpio) {}

bool PS2Mouse::waitDataAvailableWithTimeout(uint8_t size, uint16_t timeout) {
    while (available() < size && --timeout) {
        sleep_ms(50);
    }
    return timeout > 0;
}

bool PS2Mouse::enable() {
    putData(ENABLE);
    bool timedOut = !waitDataAvailableWithTimeout(1, 10);
    uint8_t r0 = getData();
    return !timedOut && r0 == ACK_OK;
}

bool PS2Mouse::reset() {
    putData(RESET);
    bool timedOut = !waitDataAvailableWithTimeout(2, 25);
    if (!timedOut) {
        uint8_t r0 = getData();
        uint8_t r1 = getData();
        m_fifo.clear();
        return r0 == ACK_OK;
    } else {
        return false;
    }
}

bool PS2Mouse::setSampleRate(uint8_t sampleRate) {
    putData(SET_SAMPLE_RATE);
    bool timedOut = !waitDataAvailableWithTimeout(1, 10);
    if (!timedOut && getData() == ACK_OK) {
        putData(sampleRate);
        timedOut = !waitDataAvailableWithTimeout(1, 10);
        if (!timedOut && getData() == ACK_OK) {
            return true;
        }
    }
    return false;
}

bool PS2Mouse::init() {
    if (PS2Device::init()) {
        if (reset()) {
            if (enable()) {
                if (setSampleRate(20)) {
                    m_fifo.clear();
                    return true;
                } else {
                    PS2_DEBUG_LOG(L_ERROR, "Error setting sample rate\n");
                }
            } else {
                PS2_DEBUG_LOG(L_ERROR, "Error enabling mouse\n");
            }
        } else {
            PS2_DEBUG_LOG(L_ERROR, "Error resetting mouse\n");
        }
    } else {
        PS2_DEBUG_LOG(L_ERROR, "Error initializing PS2 device\n");
    }
    return false;
}