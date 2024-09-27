#ifndef PIO_CONTEXT_H
#define PIO_CONTEXT_H

#include "hardware/pio.h"

struct PIOContext {
    uint sm;
    CircularBuffer<uint8_t> *fifo;
};
#endif //PIO_CONTEXT_H