#include "DB9Joystick.h"
#include "Configuration.h"
#include "hardware/gpio.h"

using namespace calypso;

DB9Joystick::DB9Joystick():
    m_joyDown(Configuration::GPIO_JOY_DOWN),
    m_joyUp(Configuration::GPIO_JOY_UP),
    m_joyLeft(Configuration::GPIO_JOY_LEFT),
    m_joyRight(Configuration::GPIO_JOY_RIGHT),
    m_joyFire1(Configuration::GPIO_JOY_FIRE1),
    m_joyFire2(Configuration::GPIO_JOY_FIRE2),
    m_maskDown(Configuration::MASK_JOY_DOWN),
    m_maskUp(Configuration::MASK_JOY_UP),
    m_maskLeft(Configuration::MASK_JOY_LEFT),
    m_maskRight(Configuration::MASK_JOY_RIGHT),
    m_maskFire1(Configuration::MASK_JOY_FIRE1),
    m_maskFire2(Configuration::MASK_JOY_FIRE2) {}

DB9Joystick::DB9Joystick(uint8_t joyLeft, uint8_t joyRight, uint8_t joyUp, uint8_t joyDown, uint8_t joyFire1, uint8_t joyFire2,
        uint8_t maskLeft, uint8_t maskRight, uint8_t maskUp, uint8_t maskDown, uint8_t maskFire1, uint8_t maskFire2):
    m_joyDown(joyDown),
    m_joyUp(joyUp),
    m_joyLeft(joyLeft),
    m_joyRight(joyRight),
    m_joyFire1(joyFire1),
    m_joyFire2(joyFire2),
    m_maskDown(maskDown),
    m_maskUp(maskUp),
    m_maskLeft(maskLeft),
    m_maskRight(maskRight),
    m_maskFire1(maskFire1),
    m_maskFire2(maskFire2) {}        

bool DB9Joystick::init() {
    uint8_t gpios[] = {m_joyDown, m_joyUp, m_joyLeft, m_joyRight, m_joyFire1, m_joyFire2};
    for (int i = 0; i < sizeof(gpios); i++) {
        gpio_init(gpios[i]);
        gpio_set_dir(gpios[i], GPIO_IN);
        gpio_pull_up(gpios[i]);
    }
    return true;
}

uint8_t DB9Joystick::read() {
    uint8_t data = 0;
    data |= gpio_get(m_joyLeft)  ? 0 : m_maskLeft;
    data |= gpio_get(m_joyRight) ? 0 : m_maskRight;
    data |= gpio_get(m_joyUp)    ? 0 : m_maskUp;
    data |= gpio_get(m_joyDown)  ? 0 : m_maskDown;
    data |= gpio_get(m_joyFire1) ? 0 : m_maskFire1;
    data |= gpio_get(m_joyFire2) ? 0 : m_maskFire2;
    return data;
}