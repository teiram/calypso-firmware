#include "PS2Device.h"

using namespace calypso;

inline static uint8_t decode(uint32_t value) {
    /*
     * XXXXXXXXXX DC  DC  DCDCDCDCDCDCDCDC   DC
     * |          |   |   |                  |----------------> start bit
     * |          |   |   |-----------------------------------> data bits
     * |          |   |---------------------------------------> parity
     * |          |-------------------------------------------> stop bit
     * |------------------------------------------------------> filler
     *                    D D D D D D D D  -------------------> data bits to keep
     */
    uint8_t decoded = 0;
    for (uint8_t i = 0; i < 8; i++) {
        value >>= 2;
        decoded |= (value & 0x02) ? (1 << i) : 0;
    }
    return decoded;
}

static void irqCallback(void) {
    for (int i = 0; i < 2; i++) {
        PIOProgram *program = pioPrograms[i];
        if (program) {
            while (!pio_sm_is_rx_fifo_empty(program->pio, program->sm) && !program->fifo->full()) {
                uint32_t value = pio_sm_get_blocking(program->pio, program->sm);
                //Value needs to be decoded (PIO collects CLK and DATA samples)
                program->fifo->write(decode(value));
            }
            pio_interrupt_clear(program->pio, program->irqNumber);
        }
    }
}

PS2Device::PS2Device(PIOProgram& pio, CircularBuffer<uint8_t>& fifo, uint8_t clkGpio):
    m_pio(pio),
    m_fifo(fifo),
    m_clkGpio(clkGpio) {}

bool PS2Device::init() {
    /* dataPin and clkPin must be consecutive GPIOs */
    if (pio_add_program_at_offset(m_pio.pio, m_pio.program, m_pio.offset) >= 0) {
        m_pio.init(m_pio.pio, m_pio.sm, m_pio.offset, m_clkGpio);
        irq_set_exclusive_handler(m_pio.irqNumber, irqCallback);
        pio_set_irq0_source_enabled(m_pio.pio, m_pio.interruptSource, true);
        irq_set_enabled(m_pio.irqNumber, true);
        return true;
    } else {
        return false;
    }
}

void PS2Device::disable() {
     pio_sm_set_enabled(m_pio.pio, m_pio.sm, false);
}

bool PS2Device::hasData() {
    return m_fifo.available() > 0;
}

uint8_t PS2Device::getData() {
    if (m_fifo.available() > 0) {
        uint8_t value = *m_fifo.head();
        m_fifo.pop();
        return value;
    } else {
        return 0xff;
    }
}

inline uint8_t PS2Device::parity(uint8_t value) {
    value ^= value >> 4;
    value ^= value >> 2;
    value ^= value >> 1;
    return (value & 1) ^ 1;
}

void PS2Device::putData(uint8_t value) {
    if (!pio_sm_is_tx_fifo_full(m_pio.pio, m_pio.sm)) {
      pio_sm_put_blocking(m_pio.pio, m_pio.sm, value | (parity(value) << 8));
    }
}