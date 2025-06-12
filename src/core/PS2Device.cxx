#include "PS2Device.h"
#include "ps2.pio.h"
#include "calypso-debug.h"
#include "pico/time.h"

using namespace calypso;

bool PS2Device::PIO_INITIALIZED = false;

static uint8_t decode(uint32_t value) {
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

#define USE_PIO_INTERRUPTS 1

#ifdef USE_PIO_INTERRUPTS
static void ps2IrqCallback(void) {
    for (int i = 0; i < 2; i++) {
        PIOContext *context = pioContexts[i];
        if (context) {
            while (!pio_sm_is_rx_fifo_empty(pio0, context->sm)) {
                uint32_t value = pio_sm_get_blocking(pio0, context->sm);
                if (!context->fifo->full()) {
                    //Value needs to be decoded (PIO collects CLK and DATA samples)

                    context->fifo->write(decode(value));
                }
            }
        }
    }
    pio_interrupt_clear(pio0, PIO0_IRQ_0);
}
#endif

PS2Device::PS2Device(PIOContext& pio, CircularBuffer<uint8_t>& fifo, uint8_t clkGpio):
    m_pio(pio),
    m_fifo(fifo),
    m_clkGpio(clkGpio) {}

bool PS2Device::init() {


    if (!PIO_INITIALIZED) {
        if (pio_add_program_at_offset(pio0, &ps2_program, 0) >= 0) {
#ifdef USE_PIO_INTERRUPTS
            irq_add_shared_handler(PIO0_IRQ_0, ps2IrqCallback, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
            irq_set_enabled(PIO0_IRQ_0, true);
#endif
            PIO_INITIALIZED = true;
        } else {
            PS2_DEBUG_LOG(L_ERROR, "Error adding PIO program\n");
            return false;
        }
    }
#ifdef USE_PIO_INTERRUPTS   
    pio_set_irq0_source_enabled(pio0, pio_get_rx_fifo_not_empty_interrupt_source(m_pio.sm), true);
#endif
    ps2_program_init(pio0, m_pio.sm, 0, m_clkGpio);
    gpio_pull_up(m_clkGpio);
    gpio_pull_up(m_clkGpio + 1);
    return true;
}

void PS2Device::disable() {
     pio_sm_set_enabled(pio0, m_pio.sm, false);
}

uint8_t PS2Device::available() {
#ifndef USE_PIO_INTERRUPTS
    while (!pio_sm_is_rx_fifo_empty(pio0, m_pio.sm) && !m_fifo.full()) {
        uint32_t value = pio_sm_get_blocking(pio0, m_pio.sm);
        //Value needs to be decoded (PIO collects CLK and DATA samples)
        m_fifo.write(decode(value));
    }
#endif
    return m_fifo.available();
}

bool PS2Device::hasData() {
    return available() > 0;
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
    if (!pio_sm_is_tx_fifo_full(pio0, m_pio.sm)) {
        gpio_pull_up(m_clkGpio);
        gpio_pull_up(m_clkGpio + 1);
        pio_sm_put_blocking(pio0, m_pio.sm, value | (parity(value) << 8));
    }
}

bool PS2Device::waitDataAvailableWithTimeout(uint8_t size, uint16_t timeout) {
    while (available() < size && --timeout) {
        sleep_ms(50);
    }
    return timeout > 0;
}
