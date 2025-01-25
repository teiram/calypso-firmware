#include <string.h>
#include "MistService.h"
#include "MistHIDUSBController.h"
#include "MistUSBJoystickHandler.h"
#include "MistUSBKeyboardHandler.h"
#include "MistUSBMouseHandler.h"
#include "compat.h"
#include "hardware/gpio.h"

using namespace calypso;

MistUSBJoystickHandler usbJoystickHandler;
MistUSBKeyboardHandler usbKeyboardHandler;
MistUSBMouseHandler usbMouseHandler;
MistHIDUSBController mistHIDUSBController(&usbKeyboardHandler,
    &usbMouseHandler,
    &usbJoystickHandler);

MistService::MistService(uint8_t userIOGPIO, uint8_t dataIOGPIO, uint8_t osdGPIO, uint8_t directModeGPIO):
    m_userIOGPIO(userIOGPIO),
    m_dataIOGPIO(dataIOGPIO),
    m_osdGPIO(osdGPIO),
    m_directModeGPIO(directModeGPIO) {}

const char *MistService::name() {
    return NAME;
}

inline void MistService::initializeSelectGPIO(uint8_t gpio) {
    gpio_init(gpio);
    gpio_put(gpio, 1);
    gpio_set_dir(gpio, GPIO_OUT);
}

bool MistService::init() {
    initializeSelectGPIO(m_userIOGPIO);
    initializeSelectGPIO(m_dataIOGPIO);
    initializeSelectGPIO(m_osdGPIO);
    initializeSelectGPIO(m_directModeGPIO);

    mist_init();
    return true;
}

bool MistService::needsAttention() {
    return true;
}

void MistService::attention() {
    mist_loop();
}

void MistService::cleanup() {

}
