#include "PulseRenderer.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <cstring>
#include "calypso-debug.h"

using namespace calypso;

static int64_t isr_handler(alarm_id_t alarm_id, void *user_data) {
    PulseRenderer* p = (PulseRenderer*) user_data;
    p->isrHandler();
    return 0;
}

PulseRenderer::PulseRenderer(ConcurrentCircularBuffer<Transition>& transitionBuffer,
    uint8_t gpio, bool initialLevel, bool invertedOutput):
    m_transitionBuffer(transitionBuffer),
    m_gpio(gpio),
    m_level(initialLevel),
    m_invertedOutput(invertedOutput),
    m_enabled(false),
    m_isrEnabled(false) {
}

void PulseRenderer::enable() {
    gpio_init(m_gpio);
    gpio_set_dir(m_gpio, true);
    gpio_put(m_gpio, true);
    m_enabled = true;
}

void PulseRenderer::disable() {
    m_enabled = false;
}

bool PulseRenderer::full() {
    return m_transitionBuffer.full();
}

void PulseRenderer::write(const Transition &transition) {
    m_transitionBuffer.write(transition);

    if (m_transitionBuffer.full() && m_enabled && !m_isrEnabled) {
        TAPE_DEBUG_LOG(L_DEBUG, "** Enabling Pulse ISR **\n");
        m_isrEnabled = true;
        isrHandler();
    }
}

void PulseRenderer::isrHandler() {
    if (m_isrEnabled) {
        if (alarm_pool_add_alarm_in_us(alarm_pool_get_default(), m_nextIsrTime == 0 ? ONE_SECOND : m_nextIsrTime, isr_handler, this, false) < 0) {
            TAPE_DEBUG_LOG(L_ERROR, "Error setting alarm for next transition\n");
        }
    }
    gpio_put(m_gpio, m_invertedOutput ^ m_level);
 
    if (m_transitionBuffer.available()) {
        Transition duration(*m_transitionBuffer.head());
        if (duration.flags & VALUE_PULSE) {
            if (m_visited++ == 1) {
                m_transitionBuffer.pop();
                m_visited = 0;
            }
        } else if ((duration.flags & END_PAUSE)) {
            if (m_visited++ == 0) {
                duration.value = 1500;
                duration.flags = 0;
                m_transitionBuffer.head()->value -= 1; //Compensate real pause transition
            } else {
                m_visited = 0;
                m_transitionBuffer.pop();
            }
        } else {
            m_transitionBuffer.pop();
        }

        if (duration.flags & END_PAUSE) {
            m_nextIsrTime = ((uint32_t) duration.value) * 1000;
            m_level = 0;
        } else {
            m_nextIsrTime = (uint32_t) duration.value;
            if (duration.flags & VALUE_FORCE_ZERO) {
                m_level = 0;
            } else if (duration.flags & VALUE_FORCE_ONE) {
                m_level = 1;
            } else if (!(duration.flags & VALUE_KEEP_POL)) {
                m_level = !m_level;
            }
        }
    } else if (!m_enabled) {
        if (m_visited++ == 0) {
            m_nextIsrTime = ONE_SECOND;
            m_level = 0;
        } else {
            TAPE_DEBUG_LOG(L_DEBUG, "** Disabling Pulse ISR **\n");
            m_isrEnabled = false;
        }
    } else {
        // Buffer underrun
        TAPE_DEBUG_LOG(L_WARN, "Buffer underrun\n");
        m_nextIsrTime = ONE_SECOND;
    }
}
