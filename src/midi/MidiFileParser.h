#ifndef MIDI_FILE_PARSER_H
#define MIDI_FILE_PARSER_H

#include <cinttypes>
#include "Stream.h"

namespace calypso {

    // Where a parsed MIDI message goes. Kept abstract (rather than taking a
    // MIDIService) so the parser carries no pico/audio dependency and stays host
    // testable, the same way MidiStreamParser does.
    class MidiSink {
    public:
        virtual ~MidiSink() = default;
        virtual void midiByte(uint8_t value) = 0;
    };

    // Reads a Standard MIDI File (.mid) from a Stream and replays its messages
    // over the caller's wall clock. A MIDI player is a menu over a file, so this
    // is the MIDI counterpart of the tape parsers: a Stream in, a byte sink out.
    //
    // Only the metrical header (ticks per quarter note) is supported. Format 0
    // (one track) and format 1 (several simultaneous tracks) are both handled:
    // the tracks are merged on a single tick timeline, so a tempo change in any
    // track applies to every event that follows it, which is what a format 1
    // file means. SMPTE time division (frame/second based) is rejected.
    //
    // Forwarded messages are the ones the players understand -- note on/off,
    // control change, program change and pitch bend -- always with an explicit
    // status byte, so playback never depends on the running status the receiving
    // MidiStreamParser happens to be holding. Tempo meta events are consumed to
    // drive the clock; every other meta event, sysex and aftertouch/pressure is
    // read through and dropped.
    class MidiFileParser {
    public:
        static constexpr uint8_t MAX_TRACKS = 32;

    private:
        static constexpr const char *TYPE = {"MID"};
        // 120 bpm, the MIDI default, until the first tempo meta event.
        static constexpr uint32_t DEFAULT_US_PER_QUARTER = 500000;

        enum EventType : uint8_t {
            EVENT_NONE,
            EVENT_MESSAGE,  // channel message to forward
            EVENT_TEMPO,    // tempo meta event
            EVENT_SKIP      // meta/sysex/aftertouch read through, not forwarded
        };

        struct Track {
            uint32_t start;         // offset of the first event byte
            uint32_t end;           // offset one past the last event byte
            uint32_t cursor;        // read position while loading events
            uint32_t tick;          // absolute tick of the last event loaded
            uint8_t runningStatus;  // last channel status seen on this track
            bool loaded;            // a pending event is buffered below
            bool ended;
            uint8_t pendingType;    // EventType
            uint32_t pendingTick;   // absolute tick of the pending event
            uint32_t pendingTempo;  // microseconds per quarter, for EVENT_TEMPO
            uint8_t pendingLength;  // bytes in pendingPayload
            uint8_t pendingPayload[3];
        };

        uint8_t m_numTracks = 0;
        Track m_tracks[MAX_TRACKS];
        uint16_t m_division = 96;      // ticks per quarter note
        uint32_t m_usPerQuarter = DEFAULT_US_PER_QUARTER;
        uint32_t m_currentTick = 0;
        uint64_t m_currentTimeUs = 0;
        uint64_t m_startUs = 0;
        bool m_started = false;
        bool m_ended = false;
        // Wall-clock instant of the next event, cached so the tick->time
        // division (software on the RP2040) is not repeated on every idle pass
        // of the main loop.
        uint32_t m_scheduledTick = 0;
        uint64_t m_scheduledTimeUs = 0;
        bool m_scheduledValid = false;
        char m_statusBuffer[40];

        void resetPlayback();
        bool loadNext(Stream &stream, Track &track);
        bool readByte(Stream &stream, uint8_t &value);
        bool readVlq(Stream &stream, uint32_t &value);

    public:
        MidiFileParser();

        // Parse the header and index the track chunks. Returns false for a file
        // that is not a supported Standard MIDI File.
        bool insert(Stream &stream);
        // Return to the first event of the performance.
        void rewind(Stream &stream);
        // Mark the instant playback began; step() measures from here.
        void start(uint64_t nowUs);
        // Emit every message whose scheduled time has arrived. Returns false when
        // the file has been played to its end.
        bool step(uint64_t nowUs, Stream &stream, MidiSink &sink);

        const char *type() const;
        const char *currentStatus();
        bool ended() const;
        uint8_t numTracks() const;
        // How much of the file has been played, 0-100. Cheap and approximate:
        // it is the fraction of the track chunks' bytes already consumed, which
        // follows the music closely enough for a progress bar without reading
        // ahead (long rests cost few bytes, so this can drift a little).
        uint8_t progress() const;
    };

}

#endif //MIDI_FILE_PARSER_H
