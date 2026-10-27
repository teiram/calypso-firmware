#ifndef MIDI_SERVICE_H
#define MIDI_SERVICE_H

#include "Service.h"
#include "AudioTarget.h"
#include "Player.h"
#include "MidiStreamParser.h"
#include "NoteWatchdog.h"

namespace calypso {
    // Bridges the incoming MIDI byte stream to a Player. The service owns the
    // audio target's lifetime (init) and pulls one audio block per attention();
    // the Player owns the synthesis itself. Which Player it drives is decided by
    // the caller (main.cxx), so a different engine can be plugged in there.
    //
    // The service is brought up lazily by the mist receiver on the first MIDI
    // byte, because its output pins are shared with the tape, and can be parked
    // again when an FPGA core is reloaded and may want those pins for itself
    // (stop()). A later byte resumes it. It also bounds a held note: nothing in
    // the MIDI stream does, so if the stream stops mid-note the watchdog lets the
    // keys go rather than leaving them sounding forever.
    class MIDIService: public Service {
        private:
        // The audio target's bring-up may only be attempted once in a boot (pico_audio_i2s
        // claims its PIO state machine, DMA channel and shared IRQ handler there), so the
        // outcome is latched and NOT_TRIED is the only state that may start it.
        enum class BringUp : uint8_t { NOT_TRIED, FAILED, DONE };
        AudioTarget& m_audioTarget;
        Player& m_player;
        MidiStreamParser m_midiParser;
        NoteWatchdog m_watchdog;
        BringUp m_bringUp = BringUp::NOT_TRIED;
        bool m_running = false;
        bool m_registered = false;
        // A stop parks the audio one queue-drain period later, not at once:
        // audio_i2s_set_enabled(false) does not flush the pool, so the audio
        // already rendered at stop time (~100 ms, a full queue) would sit
        // frozen and replay over the next thing that resumed the pins. During
        // the window the silenced synth refills the queue with silence while
        // the DMA drains the stale buffers, and the next start resumes onto
        // silence. Any byte or start() cancels the pending park.
        bool m_parking = false;
        uint32_t m_parkAtMs = 0;
        uint32_t m_parkDrainMs = 0;

        public:
        constexpr static const char NAME[] = {"MIDIService"};
        MIDIService(AudioTarget &audioTarget, Player &player);
        const char* name();
        bool init();
        void cleanup();
        bool needsAttention();
        void attention();
        void accept(uint8_t midiByte);
        // One-shot bring-up plus registration, for callers that produce MIDI
        // without an FPGA byte ever arriving (the file player). Idempotent; safe
        // to call before init(). Registration is deferred to here rather than
        // boot because the boot init loop would run init() and claim the pins
        // the tape also wants.
        bool start();
        // Camera-shutter style reset of the idle watchdog. A file player knows
        // its stream is alive even while a long note sustains and no message is
        // being sent, so it pings this so the watchdog cannot cut the note. A
        // no-op while parked.
        void keepAlive();
        // Silence everything and hand the audio pins back, for an FPGA core reload
        // that may want them for something else. The park itself is deferred by one
        // queue-drain period (see m_parking). Safe before init(). Accepting a byte
        // cancels the drain and brings the audio back.
        void stop();
        // Fresh-song reset of the channel state on both sides of the bridge: the
        // player's program/volume/pan/bend and the stream parser's RPN selection.
        // The file player calls this when a song is inserted or rewound, so a song
        // always starts from the GM power-up defaults rather than the previous
        // song's leftovers. Not a pause/resume path: play() never resets.
        void resetChannels();
    };
}

#endif //MIDI_SERVICE_H
