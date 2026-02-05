#ifndef CONCURRENT_CIRCULAR_BUFFER_H
#define CONCURRENT_CIRCULAR_BUFFER_H

#include "CircularBuffer.h"
#include "pico/sync.h"

namespace calypso {
    template <typename T> class ConcurrentCircularBuffer: public CircularBuffer<T> {
        public:
            ConcurrentCircularBuffer(T buffer[], uint8_t size):
                CircularBuffer<T>(buffer, size) {
                critical_section_init(&m_criticalSection);
            }
            
            void write(const T& value) {
                critical_section_enter_blocking(&m_criticalSection);
                CircularBuffer<T>::write(value);
                critical_section_exit(&m_criticalSection);
            }

            void pop() {
                critical_section_enter_blocking(&m_criticalSection);
                CircularBuffer<T>::pop();
                critical_section_exit(&m_criticalSection);
            }
        private:
            critical_section m_criticalSection;
    };
}
#endif //CONCURRENT_CIRCULAR_BUFFER_H
