#include <stdio.h>
#include "pico/stdlib.h"

#include "Configuration.h"
#include "SPI.h"
#include "JTAG.h"
#include "Joystick.h"
#include "SPISDCard.h"
#include "Service.h"
#include "PS2Device.h"
#include "mist/MistService.h"
#include "USBService.h"
#include "HIDUSBController.h"
#include "ps2.pio.h"

using namespace calypso;

SPI spi(spi0, Configuration::GPIO_SPI_SCK, Configuration::GPIO_SPI_MISO, Configuration::GPIO_SPI_MOSI);

JTAG jtag(Configuration::GPIO_JTAG_TDO, Configuration::GPIO_JTAG_TDI,
    Configuration::GPIO_JTAG_TCK, Configuration::GPIO_JTAG_TMS);

Joystick joystick;

SPISDCard sdcard(spi, Configuration::GPIO_SD_SEL);

uint8_t kbdBuffer[8];
CircularBuffer<uint8_t> kbdFifo(kbdBuffer, 8);
PIOProgram kbdPio = {
    .pio = pio0_hw,
    .sm = 0,
    .program = &ps2_program,
    .interruptSource = pis_sm0_rx_fifo_not_empty,
    .irqNumber = PIO0_IRQ_0,
    .offset = 0,
    .init = ps2_program_init,
    .fifo = &kbdFifo
};
PS2Device keyboard(kbdPio, kbdFifo, Configuration::GPIO_PS2_CLK1);

PIOProgram* pioPrograms[] = {
    &kbdPio,
    NULL
};

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
        sleep_ms(1);
    }
}
