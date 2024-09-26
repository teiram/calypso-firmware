#include "JTAG.h"

#include "hardware/gpio.h"

using namespace calypso;

JTAG::JTAG(uint8_t tdo, uint8_t tdi, uint8_t tck, uint8_t tms):
    m_tdo(tdo),
    m_tdi(tdi),
    m_tck(tck),
    m_tms(tms) {}

void JTAG::enable() {
    gpio_init(m_tdo);
    gpio_init(m_tdi);
    gpio_init(m_tck);
    gpio_init(m_tms);

    gpio_set_dir(m_tdo, GPIO_IN);
    gpio_set_dir(m_tdi, GPIO_OUT);
    gpio_set_dir(m_tck, GPIO_OUT);
    gpio_set_dir(m_tms, GPIO_OUT);
}

void JTAG::disable() {
    //Desperate attempts to release the JTAG bus so that
    //the CYC1000 FTDI is able to manage it
    
    gpio_put(m_tdi, true);
    gpio_put(m_tck, false);
    gpio_put(m_tms, true);

    gpio_set_dir(m_tdi, GPIO_IN);
    gpio_set_dir(m_tck, GPIO_IN);
    gpio_set_dir(m_tms, GPIO_IN);
    gpio_set_input_enabled(m_tdi, false);
    gpio_set_input_enabled(m_tck, false);
    gpio_set_input_enabled(m_tms, false);
    gpio_set_input_enabled(m_tdo, false);

    gpio_set_pulls(m_tdi, false, true);
    gpio_set_pulls(m_tck, false, true);
    gpio_set_pulls(m_tms, false, true);
    gpio_set_pulls(m_tdo, false, true);
}

inline void JTAG::tck() {
    gpio_put(m_tck, 1);
    gpio_put(m_tck, 0);
}

inline void JTAG::tmsPush(bool value) {
    gpio_put(m_tms, value);
    tck();
}

inline void JTAG::tdiPush(bool value) {
    gpio_put(m_tdi, value);
    tck();
}

void JTAG::reset() {
    gpio_put(m_tms, 1);
          
    // go to reset state   
    for (int i=0; i<10; i++) {
        tck();
    }
}

inline void JTAG::enterSelectDr() {
    tmsPush(0);
    tmsPush(1); 
} 
 
inline void JTAG::enterShiftIr() {
    tmsPush(1);
    tmsPush(0);
    tmsPush(0);   
}

inline void JTAG::enterShiftDr() {
    tmsPush(0);
    tmsPush(0);
}

inline void JTAG::exitShift() {
    tmsPush(1);
    tmsPush(1);
    tmsPush(1);
}

void JTAG::read(idcode_t* value, int length) {
    int bitoffset = 0;
    uint32_t temp;

    length -= 1;
    while (length--) {
        gpio_put(m_tck, 1);
        temp = gpio_get(m_tdo) << bitoffset;
        value->code |= temp;
        gpio_put(m_tck, 0);
        bitoffset++;
    }
    gpio_put(m_tms, 1);
    gpio_put(m_tck, 1);
    temp = gpio_get(m_tdo) << bitoffset;
    value->code |= temp;
    gpio_put(m_tck, 0);

    /* Back to Select-DR */
    tmsPush(1);
    tmsPush(1);
}

void JTAG::readDr(int length) {
    enterShiftDr();
    idcode_t value = {0};
    read(&value, length);
}

int JTAG::getChainLength() {
    int i;
    gpio_put(m_tdi, 0);
    for (i = 0; i < MAX_IR_CHAINLENGTH; i++) {
        tmsPush(0);
    }
    gpio_put(m_tck, 0);
    gpio_put(m_tdi, 1);
    for (int i = 0; i < MAX_IR_CHAINLENGTH; i++) {
        gpio_put(m_tck, 1);
        if (gpio_get(m_tdo)) break;
        gpio_put(m_tck, 0);
    }
    exitShift();
    return i;
}

void JTAG::startProgram() {
    enable();
    reset();
    enterSelectDr();
    enterShiftIr();

    tdiPush(0);
    tdiPush(1);
    tdiPush(0);
    tdiPush(0);

    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    gpio_put(m_tdi, 0);

    tmsPush(1);
    tmsPush(1);
 
    gpio_put(m_tdi, 1);
    tmsPush(1);

    tmsPush(0);
    tmsPush(0);
    
    gpio_put(m_tdi, 1);

    for (int n = 0; n < 5000; n++) {
        tck();
    }
    gpio_put(m_tdi, 0);
}

void JTAG::endProgram() {
    // Exit DR state
    tmsPush(1);
    // Update DR state
    tmsPush(0);
    // Run/Idle state
    enterSelectDr();
    enterShiftIr();
    // Shift IR state
    
    tdiPush(0);
    tdiPush(0);
    tdiPush(1);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    gpio_put(m_tdi, 0);
    tmsPush(1);
    // Exit IR state
    tmsPush(1);
    tmsPush(1);
    // Select DR scan state
    tmsPush(1);
    tmsPush(0);
    tmsPush(0);
    // Shift IR state
    tdiPush(1);
    tdiPush(1);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    tdiPush(0);
    gpio_put(m_tdi, 0);
    tmsPush(1);
    //Exit IR state
    tmsPush(1);
    tmsPush(0);
    //Idle state
    for (int n = 0; n < 200; n++) {
        tck();
    }
    reset();
    disable();
}

void JTAG::programPostamble() {
    for (int i = 0; i < 127; i++) {
        tdiPush(1);
    }
    gpio_put(m_tdi, 1);
    tmsPush(1);
}

void JTAG::programChunk(uint8_t *data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        uint8_t value = data[i];
        for (int j = 0; j < 8; j++) {
            tdiPush(value & 1);
            value >>= 1;
        }
    }
}

int JTAG::scan() {
    reset();
    enterSelectDr();
    enterShiftIr();
    int irlen = getChainLength();
    enterShiftDr();
    int device_count = getChainLength();

    if (irlen < MAX_IR_CHAINLENGTH && device_count < MAX_IR_CHAINLENGTH) {
        reset();
        enterSelectDr();
        readDr(32 * device_count);
        return 0;
    } else {
        return 1;
    }
}