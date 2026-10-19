#ifndef MIDI_PLAYER_SERVICE_H
#define MIDI_PLAYER_SERVICE_H

#include "Service.h"
#include "Stream.h"
#include "MidiFileParser.h"
#include "MIDIService.h"

namespace calypso {

    // Plays a Standard MIDI File into the MIDI service. This is the MIDI
    // counterpart of TapeService: it owns the open file, drives the parser on
    // each attention() and pushes the parser's bytes into the synth through
    // MIDIService::accept(), exactly as the FPGA's serial link would.
    //
    // Timing is the player's, not the synth's: the MIDI service has no notion of
    // song time, so the file's delta times are turned into wall-clock instants
    // (time_us_64) and each attention() releases every message that has come due.
    // Because playback feeds live MIDI, the synth's own idle watchdog keeps
    // working unchanged: a byte here is a byte there.
    class MidiPlayerService: public Service, public MidiSink {
    public:
        static constexpr const char NAME[] = {"MidiPlayerService"};

    private:
        MIDIService &m_midiService;
        Stream *m_stream = nullptr;
        MidiFileParser *m_parser = nullptr;
        bool m_play = false;

    public:
        explicit MidiPlayerService(MIDIService &midiService);
        const char *name();
        bool init();
        void cleanup();
        bool needsAttention();
        void attention();

        bool insert(Stream *stream, MidiFileParser *parser);
        bool inserted();
        bool playing();
        void play();
        void stop();
        void eject();
        void rewind();
        const char *currentStatus();
        const char *parserType();
        MidiFileParser *parser();
        uint8_t progress();

        // MidiSink: the parser's output lands here.
        void midiByte(uint8_t value);
    };

}

#endif //MIDI_PLAYER_SERVICE_H
