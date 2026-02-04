#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <inttypes.h>

template <typename T> class CircularBuffer {
public:
    CircularBuffer(T* buffer, uint8_t size):
        m_buffer(buffer),
        m_rindex(0),
        m_windex(0),
        m_size(size),
        m_full(false) {};
    
    T* head() const {
        if (m_rindex != m_windex || m_full) {
            return &m_buffer[m_rindex];
        }
        return nullptr;
    }

    void pop() {
        if (m_rindex != m_windex || m_full) {
            m_rindex = ++m_rindex % m_size;
            m_full = false;
        }
    }

    void write(const T& value) {
        if (m_rindex != m_windex || !m_full) {
            m_buffer[m_windex] = value;
            m_windex = ++m_windex % m_size;
            m_full = m_rindex == m_windex;
        }
    }

    uint8_t available() {
        if (m_rindex == m_windex) {
            return m_full ? m_size: 0;
        } else {
            if (m_rindex < m_windex) {
                return m_windex - m_rindex;
            } else {
                return m_size - m_rindex + m_windex;
            }
        }
    }

    void clear() {
        m_rindex = m_windex = 0;
    }

    bool full() {
        return m_full;
    }
private:
    T *m_buffer;
    volatile uint8_t m_rindex;
    volatile uint8_t m_windex;
    const uint8_t m_size;
    volatile bool m_full;
};
#endif //CIRCULAR_BUFFER_H