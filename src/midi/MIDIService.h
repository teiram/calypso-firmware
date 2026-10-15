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

        public:
        constexpr static const char NAME[] = {"MIDIService"};
        MIDIService(AudioTarget &audioTarget, Player &player);
        const char* name();
        bool init();
        void cleanup();
        bool needsAttention();
        void attention();
        void accept(uint8_t midiByte);
        // Let go of every sounding note and hand the audio pins back, for an FPGA core reload
        // that may want them for something else. Safe before init(). Accepting a byte brings
        // the audio back.
        void stop();
    };
}

#endif //MIDI_SERVICE_H
