#ifndef MZF_TAPE_PARSER_H
#define MZF_TAPE_PARSER_H

#include "TapeParser.h"

namespace calypso {

    class MzfTapeParser: public TapeParser {
    public:
        MzfTapeParser();
        bool insert(Stream& stream);
        void rewind(Stream& stream);
        bool needsAttention();
        void renderStep(PulseRenderer &pulseRenderer, Stream &stream);
        const char* type();
        TapeConfiguration configuration();
        const char* currentStatus();
        bool playing();
    private:
        static constexpr TapeConfiguration CONFIGURATION = {.initialLevel = true, .reverseLevel = false, .senseMotor = false};
        static constexpr uint32_t PULSE_ONE_1ST_HALF = 464;
        static constexpr uint32_t PULSE_ONE_2ND_HALF = 494;
        static constexpr uint32_t PULSE_ZERO_1ST_HALF = 240;
        static constexpr uint32_t PULSE_ZERO_2ND_HALF = 464;
        typedef enum {
            MZF_IDLE,
            MZF_INITIALIZED,

            MZF_SHORT22000,
            MZF_TAPEMARK40,
            MZF_LONG1,
            MZF_INFO1,
            MZF_INFO_SUM1,
            MZF_LONG2,
            MZF_INFO_SHORT256,
            MZF_INFO2,
            MZF_INFO_SUM2,
            MZF_LONG3,
            MZF_SHORT11000,
            MZF_TAPEMARK20,
            MZF_LONG4,
            MZF_DATA1,
            MZF_DATA_SUM1,
            MZF_LONG5,
            MZF_DATA_SHORT256,
            MZF_DATA2,
            MZF_DATA_SUM2,
            MZF_LONG6,

            MZF_ERROR
        } MzfState;
        static constexpr const char* TYPE = {"MZF"};
    
        void prepareForData(Stream &stream, MzfState nextState, uint32_t streamPos, uint32_t size);
        void sendData(Stream &stream, MzfState nextState);
        void sendSum(MzfState nextState);
        MzfState m_state;
        uint32_t m_streamSize;
        uint16_t m_pendingZeros;
        uint16_t m_pendingOnes;
        uint32_t m_currentPosition;
        uint16_t m_currentValue;
        uint8_t m_pendingBits;
        uint32_t m_pendingBytes;
        uint16_t m_crc;
        bool m_half;
        char m_statusBuffer[32];
    };
}
#endif //MZF_TAPE_PARSER_H
