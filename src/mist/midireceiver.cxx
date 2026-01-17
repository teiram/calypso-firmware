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
        midiService.init();
        Service::registerService(&midiService);
        serviceStarted = true;
    }
    
    midiService.accept(value);
}

