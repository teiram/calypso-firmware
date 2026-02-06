#ifndef C64_TAP_PARSER_H
#define C64_TAP_PARSER_H

#include "TapeParser.h"

namespace calypso {

    class C64TapParser: public TapeParser {
    private:
        static constexpr const char *TYPE = {"TAP-C64"};
        static constexpr TapeConfiguration CONFIGURATION = {.initialLevel = true, .reverseLevel = false, .senseMotor = true};
        inline uint32_t getPulseLen(uint8_t value, Stream &stream);
        uint8_t m_version;
        uint32_t m_size;
        uint32_t m_currentPosition;
        bool m_valid;
        bool m_eof;
        char m_statusBuffer[32];
    public:
        bool insert(Stream& stream);
        void rewind(Stream& stream);
        bool needsAttention();
        void renderStep(PulseRenderer &pulseRenderer, Stream &stream);
        const char* type();
        const char* currentStatus();
        bool playing();
        uint8_t numBlocks();
        uint8_t startBlock();
        void setStartBlock(uint8_t startBlock);
        TapeConfiguration configuration();
    };
}

#endif //C64_TAP_PARSER_H