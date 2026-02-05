#include "MIDIStateMachine.h"
#include "calypso-debug.h"
#include <math.h>
#include "pico/stdlib.h"


MIDIStateMachine::MIDIStateMachine() {}

void MIDIStateMachine::oscInit(const uint32_t sampleFrequency) {
    const double countInc = COUNT_INC;
    const double logNoteStepRatio = log(OCTAVE_FREQ_RATIO) / NOTES_PER_OCTAVE;
    for (size_t osc = 0; osc < NUM_OSC; osc++) {
        const double oscFrequency = A4_FREQ * exp((osc - (double) A4_NOTE_NUMBER) * logNoteStepRatio);
        // half (0.5) inc, since square wave elongation toggles twice per period
        const uint32_t countWrap = round(0.5 * countInc * sampleFrequency / oscFrequency);
        MIDIStateMachine::OscStatus *oscStatus = &m_oscStatuses[osc];
        oscStatus->countWrap = countWrap;
        oscStatus->count = 0;
        oscStatus->velocity = 0;
        oscStatus->elongation = 0;
   }
}

void MIDIStateMachine::stateInit() {
    MIDI_DEBUG_LOG(L_DEBUG, "MiDIStateMachine::stateInit\n");
    for (size_t channel = 0; channel < NUM_CHN; channel++) {
        ChannelStatus *channelStatus = &m_midiStatus.channelStatus[channel];
        for (size_t note = 0; note < NUM_OSC; note++) {
            NoteStatus *noteStatus = &channelStatus->noteStatus[note];
            noteStatus->velocity = 0;
        }
    }
    m_parserState = WAIT_CHANNEL;
}

void MIDIStateMachine::reset() {
    stateInit();
    for (size_t osc = 0; osc < NUM_OSC; osc++) {
        MIDIStateMachine::OscStatus *oscStatus = &m_oscStatuses[osc];
        oscStatus->elongation = 0;
        oscStatus->count = 0;
        oscStatus->velocity = 0;
    }
}

bool MIDIStateMachine::init(const uint32_t sampleFrequency) {
    m_timestampActingSensing = time_us_64();
    oscInit(sampleFrequency);
    stateInit();
    return true;
}

MIDIStateMachine::OscStatus *MIDIStateMachine::oscStatuses() {
    return &m_oscStatuses[0];
}

void MIDIStateMachine::addNote(const uint8_t pitch, const int8_t deltaVelocity) {
    OscStatus *oscStatus = &m_oscStatuses[pitch];
    oscStatus->velocity += deltaVelocity;
    const int16_t elongation = oscStatus->elongation;
    if (elongation > 0) {
        oscStatus->elongation += deltaVelocity;
    } else if (elongation < 0) {
        oscStatus->elongation -= deltaVelocity;
    } else {
        oscStatus->elongation = deltaVelocity;
    }
}

void MIDIStateMachine::processEvent(const MidiEvent &event) {
    MIDI_DEBUG_LOG(L_TRACE, "E: %d,%d,%d\n", event.channel, event.pitch, event.velocity);
    ChannelStatus *channelStatus = &m_midiStatus.channelStatus[event.channel];
    NoteStatus *noteStatus = &channelStatus->noteStatus[event.pitch];
    const uint8_t currentVelocity = noteStatus->velocity;

    if (event.noteOn) {
        noteStatus->velocity = event.velocity;
        addNote(event.pitch, event.velocity - currentVelocity);
    } else {
        noteStatus->velocity = 0;
        addNote(event.pitch, -currentVelocity);       
    }
}

void MIDIStateMachine::addMidiByte(uint8_t midiByte) {
    switch (m_parserState) {
        case WAIT_CHANNEL:
            //We only support channel on and channel off events (0x8x and 0x9x)
            if ((midiByte & 0xe0) == 0x80) { 
                m_currentMidiEvent.channel = midiByte & 0x0f;
                m_currentMidiEvent.noteOn = midiByte & 0x10;
                m_parserState = WAIT_PITCH;
            } else if (midiByte == 0xff) {
                reset();
            }
            break;
        case WAIT_PITCH:
            m_currentMidiEvent.pitch = midiByte & 0x7f;
            m_parserState = WAIT_VELOCITY; 
            break;
        case WAIT_VELOCITY:
            m_currentMidiEvent.velocity = midiByte & 0x7f;
            processEvent(m_currentMidiEvent);
            m_parserState = WAIT_CHANNEL;
    }
}
