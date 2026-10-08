#ifndef WAV_TAPE_PARSER_H
#define WAV_TAPE_PARSER_H

#include "TapeParser.h"

namespace calypso {

    class WavTapeParser: public TapeParser {
    public:
        WavTapeParser();
        bool insert(Stream& stream);
        void rewind(Stream& stream);
        bool needsAttention();
        void renderStep(PulseRenderer &pulseRenderer, Stream &stream);
        const char* type();
        TapeConfiguration configuration();
        const char* currentStatus();
        bool playing();
    private:
        static constexpr uint8_t HEADER_SIZE = 44;
        static constexpr TapeConfiguration CONFIGURATION = {.initialLevel = true, .reverseLevel = false, .senseMotor = false};
        static constexpr const char* TYPE = {"WAV"};
        uint32_t getNextPulseLength(Stream &stream);
        typedef enum {
            WAV_IDLE,
            WAV_INITIALIZED,
            WAV_EOF,
            WAV_ERROR
        } WavState;
        WavState m_state;
        uint32_t m_streamSize;
        uint32_t m_currentPosition;
        uint8_t m_bitsPerSample;
        uint32_t m_sampleRate;
        uint8_t m_channelCount;
        char m_statusBuffer[32];
    };
}
#endif //WAV_TAPE_PARSER_H
