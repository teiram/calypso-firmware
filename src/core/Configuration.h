#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <inttypes.h>

class Configuration {
public:
    static constexpr uint8_t GPIO_SPI_SCK  = 2;
    static constexpr uint8_t GPIO_SPI_MOSI = 3;
    static constexpr uint8_t GPIO_SPI_MISO = 4;

    static constexpr uint8_t GPIO_JTAG_TDO = 18;
    static constexpr uint8_t GPIO_JTAG_TMS = 20;
    static constexpr uint8_t GPIO_JTAG_TCK = 17;
    static constexpr uint8_t GPIO_JTAG_TDI = 19;

    static constexpr uint8_t GPIO_SD_SEL = 5;

    static constexpr uint8_t GPIO_JOY_RIGHT = 29;
    static constexpr uint8_t GPIO_JOY_LEFT  = 23;
    static constexpr uint8_t GPIO_JOY_DOWN  = 27;
    static constexpr uint8_t GPIO_JOY_UP    = 28;
    static constexpr uint8_t GPIO_JOY_FIRE1 = 26;
    static constexpr uint8_t GPIO_JOY_FIRE2 = 22;

    static constexpr uint8_t GPIO_PS2_CLK1  = 10;
    static constexpr uint8_t GPIO_PS2_DATA1 = 11;
    static constexpr uint8_t GPIO_PS2_CLK2  = 8;
    static constexpr uint8_t GPIO_PS2_DATA2 = 9;

    static constexpr uint8_t GPIO_MIST_USERIO = 6;
    static constexpr uint8_t GPIO_MIST_DATAIO = 7;
    static constexpr uint8_t GPIO_MIST_OSD    = 16;
    static constexpr uint8_t GPIO_MIST_DMODE  = 13;

    static constexpr uint8_t GPIO_I2S_DATA = 12;
    static constexpr uint8_t GPIO_I2S_BASE = 14; //Takes GPIOs 14 and 15
    static constexpr uint32_t MIDI_SAMPLE_FREQ = 24000;

    static constexpr uint8_t GPIO_USER_BUTTON = 24;

    static constexpr uint8_t MASK_JOY_RIGHT = 0x01;
    static constexpr uint8_t MASK_JOY_LEFT = 0x02;
    static constexpr uint8_t MASK_JOY_DOWN = 0x04;
    static constexpr uint8_t MASK_JOY_UP = 0x08;
    static constexpr uint8_t MASK_JOY_FIRE1 = 0x10;
    static constexpr uint8_t MASK_JOY_FIRE2 = 0x20;
};

#endif //CONFIGURATION_H