#include "Apple1BinParser.h"
#include "tzxdefinitions.h"
#include "calypso-debug.h"
#include <cstring>

using namespace calypso;

#define sinfo  info.standard_block

Apple1BinParser::Apple1BinParser():
    TzxTapeParser() {
}

const char *Apple1BinParser::type() {
    return TYPE;
}

const char *Apple1BinParser::currentStatus() {
        snprintf(m_statusBuffer, 32, "P:%ld/%ld",
        m_currentPosition,
        m_streamSize);

    return m_statusBuffer;
}

bool Apple1BinParser::insert(Stream& stream) {
    TAPE_DEBUG_LOG(L_DEBUG, "Apple1BinParser::insert\n");
    stream.seekSet(0);
    m_streamSize = stream.size();
    m_tzxState = TZX_STATE_INITIALIZED;
    return true;
}

void Apple1BinParser::rewind(Stream &stream) {
    stream.seekSet(0);
    m_currentPosition = stream.position();
    m_context.current_block = 0;
    m_tzxState = TZX_STATE_INITIALIZED;
}

void Apple1BinParser::renderStep(PulseRenderer &pulseRenderer, Stream &stream) {
    switch (m_tzxState) {
    case TZX_STATE_FINDBLOCK: {
        if (m_streamSize > 0) {
            if (stream.position() == 0) {
                m_blockInfo.sinfo.pending_bits = 0;
                m_blockInfo.sinfo.zero_length = 250;
                m_blockInfo.sinfo.one_length = 500;
                m_blockInfo.sinfo.bits_last_byte = 8;
                m_blockInfo.end_pause = 1000;
                m_blockInfo.size = m_streamSize;
                m_blockInfo.pilot_length = 500;
                m_blockInfo.sinfo.sync1_length = 250;
                m_blockInfo.sinfo.sync2_length = 250;
                m_blockInfo.sinfo.block_type = 0;
                m_blockInfo.sinfo.pilot_pulses = 16000;
                m_context.blockId = TZX_STANDARD_SPEED_BLOCK;
                m_tzxState = TZX_STATE_READBLOCK;
            } else {
                m_tzxState = TZX_STATE_IDLE;
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

