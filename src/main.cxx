#include <cstdio>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "Configuration.h"
#include "SPIDevice.h"
#include "JTAG.h"
#include "DB9Joystick.h"
#include "Button.h"
#include "SPISDCard.h"
#include "Service.h"
#include "PS2Keyboard.h"
#include "PS2Mouse.h"
#include "mist/MistService.h"
#include "USBService.h"
#include "I2SAudioTarget.h"
#include "MIDIStateMachine.h"
#include "MIDIService.h"
#include "Util.h"
#include "tape/TapeService.h"
#include "tape/TzxTapeParser.h"

using namespace calypso;

SPIDevice spi(spi0, Configuration::GPIO_SPI_SCK, Configuration::GPIO_SPI_MISO, Configuration::GPIO_SPI_MOSI);

JTAG jtag(Configuration::GPIO_JTAG_TDO, Configuration::GPIO_JTAG_TDI,
    Configuration::GPIO_JTAG_TCK, Configuration::GPIO_JTAG_TMS);

DB9Joystick joystick;

SPISDCard sdcard(spi, Configuration::GPIO_SD_SEL, Configuration::GPIO_MIST_DMODE);

uint8_t kbdBuffer[8];
CircularBuffer<uint8_t> kbdFifo(kbdBuffer, 8);
PIOContext kbdPio = {
    .sm = 0,
    .fifo = &kbdFifo
};
PS2Keyboard keyboard(kbdPio, kbdFifo, Configuration::GPIO_PS2_CLK1);

uint8_t mouseBuffer[8];
CircularBuffer<uint8_t> mouseFifo(mouseBuffer, 8);
PIOContext mousePio = {
    .sm = 1,
    .fifo = &mouseFifo
};
PS2Mouse mouse(mousePio, mouseFifo, Configuration::GPIO_PS2_CLK2);

PIOContext* pioContexts[] = {&kbdPio, &mousePio};

USBService usbService;

Button userButton(Configuration::GPIO_USER_BUTTON);
MistService mistService(Configuration::GPIO_MIST_USERIO, Configuration::GPIO_MIST_DATAIO,
    Configuration::GPIO_MIST_OSD, Configuration::GPIO_MIST_DMODE);


I2SAudioTarget i2sAudioTarget(Configuration::MIDI_SAMPLE_FREQ, 
    Configuration::GPIO_I2S_BASE, Configuration::GPIO_I2S_DATA, 0);

MIDIStateMachine midiStateMachine;
MIDIService midiService(i2sAudioTarget, midiStateMachine);

PulseRenderer::Transition tapeTransitionBuffer[TapeService::TAPE_TRANSITION_BUFFER_SIZE];
ConcurrentCircularBuffer transitionBuffer(tapeTransitionBuffer, TapeService::TAPE_TRANSITION_BUFFER_SIZE);
PulseRenderer pulseRenderer(transitionBuffer, Configuration::GPIO_TZX_OUTPUT, false, false);
TapeService tapeService(pulseRenderer);
TzxTapeParser tzxTapeParser;

#if 0
static void device_init() {
    printf("Initializing devices under core1\n");
    keyboard.init();
    mouse.init();
    printf("Devices initialized\n");
}
#endif

int main() {
    stdio_init_all();

    Service::registerService(&usbService);
    Service::registerService(&mistService);
    Service::registerService(&tapeService);
    spi.init();
    
    //Multicore initialization only works from cold reset
    //for some unknown reason
#if 1
    userButton.init();
    keyboard.init();
    mouse.init();
#else
    multicore_reset_core1();
    sleep_ms(100);
    multicore_launch_core1(&device_init);
#endif
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
