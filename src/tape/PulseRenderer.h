#ifndef PULSE_RENDERER_H
#define PULSE_RENDERER_H

#include "ConcurrentCircularBuffer.h"
#include "pico/sync.h"

namespace calypso {

    class PulseRenderer {
    public:
        static constexpr uint8_t TZX_TRANSITION_BUFFER_SIZE = 64;
        
        static constexpr uint8_t END_PAUSE = 1 << 0;
        static constexpr uint8_t VALUE_PULSE = 1 << 1;
        static constexpr uint8_t VALUE_KEEP_POL = 1 << 2;
        static constexpr uint8_t VALUE_FORCE_ZERO = 1 << 3;
        static constexpr uint8_t VALUE_FORCE_ONE = 1 << 4;
        typedef struct {
            uint16_t value;
            uint8_t flags:3;
        } __attribute__((packed)) Transition;
        PulseRenderer(ConcurrentCircularBuffer<Transition>& transitionBuffer, uint8_t gpio, bool initialLevel, bool invertedOutput);
        void enable();
        void disable();
        bool full();
        void write(const Transition &transition);
        void isrHandler();
    private:
        static constexpr uint32_t ONE_SECOND = 1000000;
        ConcurrentCircularBuffer<Transition>& m_transitionBuffer;
        uint8_t m_gpio;
        bool m_enabled;
        bool m_isrEnabled;
        bool m_level;
        bool m_invertedOutput;
        uint8_t m_visited:1;
        uint32_t m_nextIsrTime;
    };

}

#endif //PULSE_RENDERER_H