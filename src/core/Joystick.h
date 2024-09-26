#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <inttypes.h>

namespace calypso {
    class Joystick {
    private:
        uint8_t m_joyUp;
        uint8_t m_joyDown;
        uint8_t m_joyLeft;
        uint8_t m_joyRight;
        uint8_t m_joyFire1;
        uint8_t m_joyFire2;
        uint8_t m_maskUp;
        uint8_t m_maskDown;
        uint8_t m_maskLeft;
        uint8_t m_maskRight;
        uint8_t m_maskFire1;
        uint8_t m_maskFire2;
    public:
        Joystick();
        Joystick(uint8_t joyLeft, uint8_t joyRight, uint8_t joyUp, uint8_t joyDown, uint8_t joyFire1, uint8_t joyFire2,
            uint8_t maskLeft, uint8_t maskRight, uint8_t maskUp, uint8_t maskDown, uint8_t maskFire1, uint8_t maskFire2);
        bool init();
        uint8_t read();
    };
}
#endif //JOYSTICK_H