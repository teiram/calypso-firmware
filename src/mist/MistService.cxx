#include <string.h>
#include "MistService.h"
#include "compat.h"
#include "hardware/gpio.h"

using namespace calypso;

MistService::MistService(HIDUSBController& hidUSBController, uint8_t userIOGPIO, uint8_t dataIOGPIO, uint8_t osdGPIO, uint8_t directModeGPIO):
    m_usbKeyboardHandler(),
    m_usbMouseHandler(),
    m_hidUSBController(hidUSBController),
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

    m_hidUSBController.addKbdReportHandler(&m_usbKeyboardHandler);
    m_hidUSBController.addMouseReportHandler(&m_usbMouseHandler);
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
