#include <cstdio>
#include "pico/stdlib.h"

#include "Configuration.h"
#include "SPI.h"
#include "JTAG.h"
#include "Joystick.h"
#include "SPISDCard.h"
#include "Service.h"
#include "PS2Device.h"
#include "PS2Mouse.h"
#include "mist/MistService.h"
#include "USBService.h"
#include "HIDUSBController.h"

using namespace calypso;

SPI spi(spi0, Configuration::GPIO_SPI_SCK, Configuration::GPIO_SPI_MISO, Configuration::GPIO_SPI_MOSI);

JTAG jtag(Configuration::GPIO_JTAG_TDO, Configuration::GPIO_JTAG_TDI,
    Configuration::GPIO_JTAG_TCK, Configuration::GPIO_JTAG_TMS);

Joystick joystick;

SPISDCard sdcard(spi, Configuration::GPIO_SD_SEL);

uint8_t kbdBuffer[8];
CircularBuffer<uint8_t> kbdFifo(kbdBuffer, 8);
PIOContext kbdPio = {
    .sm = 0,
    .fifo = &kbdFifo
};
PS2Device keyboard(kbdPio, kbdFifo, Configuration::GPIO_PS2_CLK1);

uint8_t mouseBuffer[8];
CircularBuffer<uint8_t> mouseFifo(mouseBuffer, 8);
PIOContext mousePio = {
    .sm = 1,
    .fifo = &mouseFifo
};
PS2Mouse mouse(mousePio, mouseFifo, Configuration::GPIO_PS2_CLK2);

PIOContext* pioContexts[] = {&kbdPio, &mousePio};

USBService usbService;
HIDUSBController hidUSBController;
MistService mistService(hidUSBController, 
    Configuration::GPIO_MIST_USERIO, Configuration::GPIO_MIST_DATAIO,
    Configuration::GPIO_MIST_OSD, Configuration::GPIO_MIST_DMODE);

int main() {
    stdio_init_all();

    Service::registerService(&usbService);
    Service::registerService(&mistService);

    spi.init();
    keyboard.init();
    mouse.init();
    printf("Initializing services...\n");
    for (int i = 0; i < Service::serviceCount; i++) {
        Service *s = Service::services[i];
        printf("Initializing service %s\n", s->name());
        if (!s->init()) {
            printf("Error initializing service %s", s->name());
        }
    }
    printf(" ...initialization done\n");
    printf("Calypso Firmware main loop starting\n");
    while (true) {
        for (int i = 0; i < Service::serviceCount; i++) {
            Service *s = Service::services[i];
            if (s->needsAttention()) {
                s->attention();
            }
        }
    }
}
