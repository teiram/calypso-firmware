#include "MzfTapeParser.h"
#include "calypso-debug.h"

using namespace calypso;

MzfTapeParser::MzfTapeParser():
    m_state(MZF_IDLE),
    m_currentPosition(0),
    m_pendingOnes(0),
    m_pendingZeros(0),
    m_half(false),
    m_streamSize(0) {    
}

TapeConfiguration MzfTapeParser::configuration() {
    return CONFIGURATION;
}

const char *MzfTapeParser::type() {
    return TYPE;
}

bool MzfTapeParser::insert(Stream& stream) {
    TAPE_DEBUG_LOG(L_DEBUG, "TzxTapeParser::insert\n");
    stream.seekSet(0);
    m_streamSize = stream.size();
    m_pendingOnes = m_pendingZeros = m_currentPosition = 0;
    m_state = m_streamSize > 0 ? MZF_INITIALIZED : MZF_ERROR;
    m_half = false;
    return m_streamSize > 0;
}

const char *MzfTapeParser::currentStatus() {
        snprintf(m_statusBuffer, 32, "S: %d, P:%ld/%ld, PB:%ld",
        m_state,
        m_currentPosition,
        m_streamSize,
        m_pendingBytes);

    return m_statusBuffer;
}


bool MzfTapeParser::playing() {
    return m_state != MZF_ERROR && m_state != MZF_INITIALIZED && m_state != MZF_IDLE;
}

void MzfTapeParser::rewind(Stream &stream) {
    stream.seekSet(0);
    m_pendingOnes = m_pendingZeros = m_currentPosition = 0;
    m_half = false;
    m_state = m_streamSize > 0 ? MZF_INITIALIZED : MZF_ERROR;
}

bool MzfTapeParser::needsAttention() {
    return m_state != MZF_IDLE && m_state != MZF_ERROR;
}

void MzfTapeParser::prepareForData(Stream &stream, MzfState nextState, uint32_t streamPos, uint32_t size) {
    stream.seekSet(streamPos);
    m_crc = 0;
    m_currentValue = stream.read();
    TAPE_DEBUG_LOG(L_DEBUG, "Next data value: %16x\n", m_currentValue);
    m_currentPosition = stream.position();

    m_pendingBits = 9;
    m_pendingBytes = size;
    m_state = nextState;    
}

void MzfTapeParser::sendData(Stream &stream, MzfState nextState) {
    if (m_pendingBits--) {
        if (m_pendingBits == 8) {
            m_pendingOnes = 1;
        } else {
            if (m_currentValue & 0x80) {
                m_pendingOnes = 1;
                m_crc++;
            } else {
                m_pendingZeros = 1;
            }
            m_currentValue <<= 1;
        }
    } else {
        if (--m_pendingBytes) {
            m_pendingBits = 9;
            m_currentValue = stream.read();
            m_currentPosition++;
        } else {
            m_currentValue = m_crc;
            TAPE_DEBUG_LOG(L_DEBUG, "Next CRC value: %16x\n", m_currentValue);
            m_pendingBits = 18;
            m_state = nextState;
        }
    }
}

void MzfTapeParser::sendSum(MzfState nextState) {
    if (m_pendingBits--) {
        if (m_pendingBits == 17) {
            m_pendingOnes = 1;
        } else if (m_pendingBits == 8) {
            m_pendingOnes = 1;
        } else {
            if (m_currentValue & 0x8000) {
                m_pendingOnes = 1;
            } else {
                m_pendingZeros = 1;
            }
            m_currentValue <<= 1;
        }
    } else {
        m_pendingOnes = 1;
        m_state = nextState;
    }
}

void MzfTapeParser::renderStep(PulseRenderer &pulseRenderer, Stream &stream) {
    if (m_pendingOnes > 0 || m_pendingZeros > 0) {
        if (!pulseRenderer.full()) {
            if (m_pendingOnes) {
                pulseRenderer.write({.value = m_half ? PULSE_ONE_2ND_HALF : PULSE_ONE_1ST_HALF, .flags = 0});
                if (m_half) {
                    m_pendingOnes--;
                }
                m_half = !m_half;
            } else if (m_pendingZeros) {
                pulseRenderer.write({.value = m_half ? PULSE_ZERO_2ND_HALF : PULSE_ZERO_1ST_HALF, .flags = 0});
                if (m_half) {
                    m_pendingZeros--;
                }
                m_half = !m_half;
            }
        }
    } else {
        switch (m_state) {
            case MZF_INITIALIZED:
                m_pendingZeros = 11000; //Standard: 22000;
                m_state = MZF_SHORT22000;
                break;
            case MZF_SHORT22000:
                m_pendingOnes = 40;
                m_pendingZeros = 40;
                m_state = MZF_TAPEMARK40;
                break;
            case MZF_TAPEMARK40:
                m_pendingOnes = 1;
                m_state = MZF_LONG1;
                break;
            case MZF_LONG1:
                prepareForData(stream, MZF_INFO1, 0, 128);
                break;
            case MZF_INFO1:
                sendData(stream, MZF_INFO_SUM1);
                break;
            case MZF_INFO_SUM1:
                sendSum(MZF_LONG3); //sendSum(MZF_LONG2);
                break;
            case MZF_LONG2:
                m_pendingZeros = 256;
                m_state = MZF_INFO_SHORT256;
                break;
            case MZF_INFO_SHORT256:
                prepareForData(stream, MZF_INFO2, 0, 128);
                break;
            case MZF_INFO2:
                sendData(stream, MZF_INFO_SUM2);
                break;
            case MZF_INFO_SUM2:
                sendSum(MZF_LONG3);
                break;
            case MZF_LONG3:
                m_pendingZeros = 11000;
                m_state = MZF_SHORT11000;
                break;
            case MZF_SHORT11000:
                m_pendingOnes = 20;
                m_pendingZeros = 20;
                m_state = MZF_TAPEMARK20;
                break;
            case MZF_TAPEMARK20:
                m_pendingOnes = 1;
                m_state = MZF_LONG4;
                break;
            case MZF_LONG4:
                prepareForData(stream, MZF_DATA1, 128, m_streamSize - 128);
                break;
            case MZF_DATA1:
                sendData(stream, MZF_DATA_SUM1);
                break;
            case MZF_DATA_SUM1:
                sendSum(MZF_LONG6); //sendSum(MZF_LONG5);
                break;
            case MZF_LONG5:
                m_pendingZeros = 256;
                m_state = MZF_DATA_SHORT256;
                break;
            case MZF_DATA_SHORT256:
                prepareForData(stream, MZF_DATA2, 128, m_streamSize - 128);
                break;
            case MZF_DATA2:
                sendData(stream, MZF_DATA_SUM2);
                break;
            case MZF_DATA_SUM2:
                sendSum(MZF_LONG6);
                break;
            case MZF_LONG6:
                m_state = MZF_IDLE;
                break;
        }
    }
}