#ifndef MIDI_SERVICE_H
#define MIDI_SERVICE_H

#include "Service.h"
#include "AudioTarget.h"
#include "Player.h"
#include "MidiStreamParser.h"

namespace calypso {
    // Bridges the incoming MIDI byte stream to a Player. The service owns the
    // audio target's lifetime (init) and pulls one audio block per attention();
    // the Player owns the synthesis itself. Which Player it drives is decided by
    // the caller (main.cxx), so a different engine can be plugged in there.
    class MIDIService: public Service {
        private:
        AudioTarget& m_audioTarget;
        Player& m_player;
        MidiStreamParser m_midiParser;

        public:
        constexpr static const char NAME[] = {"MIDIService"};
        MIDIService(AudioTarget &audioTarget, Player &player);
        const char* name();
        bool init();
        void cleanup();
        bool needsAttention();
        void attention();
        void accept(uint8_t midiByte);
    };
}

#endif //MIDI_SERVICE_H
