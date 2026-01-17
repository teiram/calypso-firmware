#ifndef MIDI_SERVICE_H
#define MIDI_SERVICE_H

#include "Service.h"
#include "AudioTarget.h"
#include "MIDIStateMachine.h"

namespace calypso {
    class MIDIService: public Service {
        private:
        static constexpr uint32_t DEFAULT_SAMPLE_FREQ = 24000;
        static constexpr uint8_t VOL_BITS = 8;

        AudioTarget& m_audioTarget;
        MIDIStateMachine& m_midiStateMachine;

        public:
        constexpr static const char NAME[] = {"MIDIService"};
        MIDIService(AudioTarget &audioTarget, MIDIStateMachine &midiStateMachine);
        const char* name();
        bool init();
        void cleanup();
        bool needsAttention();
        void attention();
        void accept(uint8_t midiByte);
    };
}

#endif //MIDI_SERVICE_H