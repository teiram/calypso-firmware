#include "Button.h"
#include "Configuration.h"
#include "hardware/gpio.h"

using namespace calypso;

Button::Button(uint8_t buttonGpio):
    m_buttonGpio(buttonGpio) {}        

bool Button::init() {
    gpio_init(m_buttonGpio);
    gpio_set_dir(m_buttonGpio, GPIO_IN);
    gpio_pull_up(m_buttonGpio);
    return true;
}

bool Button::read() {
    return gpio_get(m_buttonGpio);
}