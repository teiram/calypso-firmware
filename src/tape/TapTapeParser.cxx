#include "TapTapeParser.h"
#include "tzxdefinitions.h"
#include "calypso-debug.h"
#include <cstring>

using namespace calypso;

#define sinfo  info.standard_block

TapTapeParser::TapTapeParser():
    TzxTapeParser() {
}

const char *TapTapeParser::type() {
    return TYPE;
}

bool TapTapeParser::insert(Stream& stream) {
    TAPE_DEBUG_LOG(L_DEBUG, "TzxTapeParser::insert\n");
    stream.seekSet(0);
    m_streamSize = stream.size();
    m_tzxState = TZX_STATE_INITIALIZED;
    return true;
}

void TapTapeParser::rewind(Stream &stream) {
    stream.seekSet(0);
    m_currentPosition = stream.position();
    m_context.current_block = 0;
    m_tzxState = TZX_STATE_INITIALIZED;
}

void TapTapeParser::renderStep(PulseRenderer &pulseRenderer, Stream &stream) {
    int16_t next_value;
    switch (m_tzxState) {
    case TZX_STATE_FINDBLOCK: {
        uint16_t* size = (uint16_t*) m_inputBuffer;
        if (stream.read((void*) size, 2) > -1) {
            m_blockInfo.sinfo.zero_length = STD_ZERO_LENGTH;
            m_blockInfo.sinfo.one_length = STD_ONE_LENGTH;
            m_blockInfo.sinfo.bits_last_byte = 8;
            m_blockInfo.end_pause = STD_FINAL_PAUSE;
            m_blockInfo.size = *size;
            m_blockInfo.pilot_length = STD_LEADER_LENGTH;
            m_blockInfo.sinfo.sync1_length = STD_SYNC1_LENGTH;
            m_blockInfo.sinfo.sync2_length = STD_SYNC2_LENGTH;
            next_value = stream.read(); //Get the type of block
            if (next_value > -1) {
                m_blockInfo.sinfo.block_type = next_value & 0xff;
                m_blockInfo.sinfo.pilot_pulses = m_blockInfo.sinfo.block_type < 128 ?
                    STD_HEADER_PULSES : STD_DATA_PULSES;
                m_blockInfo.size--; //Compensate for block_type prefetch
                m_context.blockId = TZX_STANDARD_SPEED_BLOCK;
                m_tzxState = TZX_STATE_READBLOCK;
            } else {
                m_tzxState = TZX_STATE_ERROR;
            }
        } else {
            m_tzxState = TZX_STATE_ERROR;
        }
        break;
    }
    default:
        TzxTapeParser::renderStep(pulseRenderer, stream);
    }
}

