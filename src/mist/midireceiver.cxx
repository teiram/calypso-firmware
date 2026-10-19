#include "midireceiver.h"
#include "Service.h"
#include "MIDIService.h"
#include <cstdio>
#include <cstdbool>

using namespace calypso;

extern MIDIService midiService;

static bool serviceStarted = false;

void midi_byte_processor(uint8_t value) {
    //Start the service on reception of MIDI data
    //Since this service needs pins shared with the PSRAM powerpack module
    
    if (!serviceStarted) {
        midiService.start();
        serviceStarted = true;
    }
    
    midiService.accept(value);
}

void midi_stop(void) {
    // serviceStarted stays true on purpose: the service object is already registered and
    // initialized, and init() must not run twice (the pico_audio_i2s setup it performs is a
    // one-shot in a boot). Accepting a byte after this resumes the parked service.
    if (serviceStarted) {
        midiService.stop();
    }
}
