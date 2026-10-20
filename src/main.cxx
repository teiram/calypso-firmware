#include <cstdio>
#include <atomic>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "hardware/clocks.h"
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
#include "midi/I2SAudioTarget.h"
#include "midi/WaveTablePlayer.h"
#include "midi/MIDIService.h"
#include "midi/MidiPlayerService.h"
#include "midi/PlayerCommandQueue.h"
#include "midi/RemotePlayer.h"
#include "Util.h"
#include "tape/TapeService.h"
#include "tape/TzxTapeParser.h"
#include "tape/TapTapeParser.h"
#include "tape/C64TapParser.h"
#include "tape/Apple1BinParser.h"
#include "tape/MzfTapeParser.h"
#include "tape/WavTapeParser.h"

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

// The synth engine. WaveTablePlayer is the default; SquareWavePlayer is kept
// buildable as the alternative and is selected here if it is ever wanted. It is
// static storage on purpose: the player owns tens of kilobytes of generator and
// reverb state that cannot live on the small main stack.
static WaveTablePlayer wavePlayer(i2sAudioTarget);
static PlayerCommandQueue midiCommandQueue;
static RemotePlayer remotePlayer(wavePlayer, midiCommandQueue);
MIDIService midiService(i2sAudioTarget, remotePlayer);
MidiPlayerService midiPlayerService(midiService);

// Core 1 audio rendering control
std::atomic<bool> audioEnabledCore1{false};

extern "C" void audio_core1_enable(bool enable) {
    audioEnabledCore1.store(enable, std::memory_order_release);
}

static void audioCore1() {
    while (true) {
        if (!audioEnabledCore1.load(std::memory_order_acquire)) {
            tight_loop_contents();
            continue;
        }
        PlayerCommand cmd;
        while (midiCommandQueue.pop(cmd)) {
            applyCommand(wavePlayer, cmd);
        }
        wavePlayer.synthTask();
    }
}

PulseRenderer::Transition tapeTransitionBuffer[TapeService::TAPE_TRANSITION_BUFFER_SIZE];
ConcurrentCircularBuffer transitionBuffer(tapeTransitionBuffer, TapeService::TAPE_TRANSITION_BUFFER_SIZE);
PulseRenderer pulseRenderer(transitionBuffer, 
    Configuration::GPIO_TAPE_OUTPUT,
    false, false);
TapeService tapeService(pulseRenderer, Configuration::GPIO_TAPE_MOTOR);
TzxTapeParser tzxTapeParser;
TapTapeParser tapTapeParser;
C64TapParser c64TapParser;
Apple1BinParser apple1BinParser;
MzfTapeParser mzfTapeParser;
WavTapeParser wavTapeParser;

int main() {
    // The wavetable mixer needs the headroom: at the default 125 MHz a full
    // 64-voice block cannot be rendered inside its 2.7 ms budget. This must come
    // before stdio_init_all() and every other peripheral init, because clk_sys
    // (and clk_peri) divide the UART, SPI and PIO rates.
    set_sys_clock_khz(250000, false);

    stdio_init_all();

    // Launch audio core early
    sleep_ms(10);
    multicore_launch_core1(&audioCore1);

    Service::registerService(&usbService);
    Service::registerService(&mistService);
    Service::registerService(&tapeService);
    Service::registerService(&midiPlayerService);
    spi.init();
    
    userButton.init();
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

