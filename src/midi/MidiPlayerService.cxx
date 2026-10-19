#include "MidiPlayerService.h"
#include "calypso-debug.h"
#include "pico/time.h"

using namespace calypso;

MidiPlayerService::MidiPlayerService(MIDIService &midiService):
    m_midiService(midiService) {
}

const char *MidiPlayerService::name() {
    return NAME;
}

bool MidiPlayerService::init() {
    // Nothing to bring up: the audio path belongs to the MIDI service and is
    // started on play(), so registering this service at boot claims no pins.
    return true;
}

void MidiPlayerService::cleanup() {
    stop();
}

bool MidiPlayerService::needsAttention() {
    return m_play;
}

void MidiPlayerService::attention() {
    if (m_parser == nullptr || m_stream == nullptr) {
        return;
    }
    // The file is the source of truth for liveness, so hold the synth's idle
    // watchdog open: a note sustained across a long rest must not be released
    // just because no new message was sent.
    m_midiService.keepAlive();
    if (!m_parser->step(time_us_64(), *m_stream, *this)) {
        // End of file: silence, release the shared audio pins, and leave the
        // file rewound for the next play.
        stop();
        m_parser->rewind(*m_stream);
    }
}

bool MidiPlayerService::insert(Stream *stream, MidiFileParser *parser) {
    MIDI_DEBUG_LOG(L_DEBUG, "MidiPlayerService::insert\n");
    if (!parser->insert(*stream)) {
        MIDI_DEBUG_LOG(L_WARN, "Unable to insert into MidiPlayerService\n");
        return false;
    }
    m_stream = stream;
    m_parser = parser;
    return true;
}

bool MidiPlayerService::inserted() {
    return m_stream != nullptr && m_parser != nullptr;
}

bool MidiPlayerService::playing() {
    return m_play;
}

void MidiPlayerService::play() {
    if (!inserted()) {
        return;
    }
    MIDI_DEBUG_LOG(L_DEBUG, "MidiPlayerService::play\n");
    // Bring the synth up on demand: a file can be the first thing that asks for
    // MIDI, in which case no FPGA byte ever registered the service.
    m_midiService.start();
    m_parser->start(time_us_64());
    m_play = true;
}

void MidiPlayerService::stop() {
    MIDI_DEBUG_LOG(L_DEBUG, "MidiPlayerService::stop\n");
    m_play = false;
    // Let go of every sounding note and hand the shared pins back.
    m_midiService.stop();
}

void MidiPlayerService::eject() {
    MIDI_DEBUG_LOG(L_DEBUG, "MidiPlayerService::eject\n");
    stop();
    if (m_stream != nullptr) {
        m_stream->close();
    }
    m_stream = nullptr;
    m_parser = nullptr;
}

void MidiPlayerService::rewind() {
    if (m_parser != nullptr && m_stream != nullptr) {
        m_parser->rewind(*m_stream);
    }
}

const char *MidiPlayerService::currentStatus() {
    return m_parser != nullptr ? m_parser->currentStatus() : "";
}

const char *MidiPlayerService::parserType() {
    return m_parser != nullptr ? m_parser->type() : "";
}

MidiFileParser *MidiPlayerService::parser() {
    return m_parser;
}

uint8_t MidiPlayerService::progress() {
    return m_parser != nullptr ? m_parser->progress() : 0;
}

void MidiPlayerService::midiByte(uint8_t value) {
    m_midiService.accept(value);
}
