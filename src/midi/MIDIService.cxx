#include "MIDIService.h"

using namespace calypso;

MIDIService::MIDIService(AudioTarget &audioTarget, Player &player):
    m_audioTarget(audioTarget),
    m_player(player),
    m_midiParser(player) {}

const char *MIDIService::name() {
    return NAME;
}

bool MIDIService::init() {
    // The pool geometry is the engine's: the wavetable mixer quantises a note
    // start to a block, so it asks for short blocks (there is no sampler here;
    // the point is the note-onset quantisation, not latency).
    if (!m_audioTarget.init(m_player.audioBufferCount(), m_player.audioBufferSamples())) {
        return false;
    }
    return m_player.init();
}

bool MIDIService::needsAttention() {
    return true;
}

void MIDIService::attention() {
    m_player.synthTask();
}

void MIDIService::accept(uint8_t midiByte) {
    m_midiParser.accept(midiByte);
}

void MIDIService::cleanup() {}
