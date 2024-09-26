#ifndef PIO_PROGRAM_H
#define PIO_PROGRAM_H

#include "hardware/pio.h"

struct PIOProgram {
    PIO pio;
    uint sm;
    const pio_program* program;
    pio_interrupt_source_t interruptSource;
    irq_num_rp2040 irqNumber;
    uint offset;
    void (*init) (PIO, uint, uint, uint8_t);
    CircularBuffer<uint8_t> *fifo;
};
#endif //PIO_PROGRAM_H