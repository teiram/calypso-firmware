#include "C64TapParser.h"
#include <cstring>
#include "calypso-debug.h"

using namespace calypso;

TapeConfiguration C64TapParser::configuration() {
    return CONFIGURATION;
}

bool C64TapParser::insert(Stream &stream) {
    char signature[12];
    stream.seekSet(0);
    int16_t value;
    
    if (stream.read(signature, 12) == 12) {
        if (!strncmp(signature, "C64-TAPE-RAW", 12)) {
            value = stream.read();
            if (value > -1) {
                m_version = (uint8_t) value;
                stream.seekSet(16);
                uint8_t sizeBytes[4];
                if (stream.read(sizeBytes, 4) == 4) {
                    m_size = sizeBytes[0] + 
                        ((uint32_t) sizeBytes[1] << 8) +
                        ((uint32_t) sizeBytes[2] << 16) +
                        ((uint32_t) sizeBytes[3] << 24);
                    m_currentPosition = 0;
                    m_valid = true;
                    m_eof = false;
                    return true;
                } else {
                    TAPE_DEBUG_LOG(L_WARN, "Unable to read size bytes\n");
                }
            } else {
                TAPE_DEBUG_LOG(L_WARN, "Unable to read version\n");
            }
        } else {
            TAPE_DEBUG_LOG(L_WARN, "Unexpected signature\n");
        }
    } else {
        TAPE_DEBUG_LOG(L_WARN, "Unable to read signature bytes\n");
    }
    return false;
}

void C64TapParser::rewind(Stream& stream) {
    if (m_valid) {
        stream.seekSet(20);
        m_currentPosition = 0;
        m_eof = false;
    }
}

bool C64TapParser::needsAttention() {
    return m_valid && !m_eof;
}

inline uint32_t zeroDurationFromStream(Stream &stream) {
    char durationBytes[3];
    stream.read(durationBytes, 3);
    return durationBytes[0] +
        ((uint32_t) durationBytes[1] << 8) +
        ((uint32_t) durationBytes[2] << 16);
}

inline uint32_t C64TapParser::getPulseLen(uint8_t value, Stream &stream) {
    if (value) {
        return value * 4; //Asuming cycles of 1us
    } else {
        switch (m_version) {
            case 0: 
            return 20000;
            default:
            return zeroDurationFromStream(stream);
        }
    }
}

void C64TapParser::renderStep(PulseRenderer &pulseRenderer, Stream &stream) {
    if (m_valid && !pulseRenderer.full()) {
        uint16_t value = stream.read();
        if (value > -1) {
            m_currentPosition++;
            uint32_t duration = getPulseLen((uint8_t) value, stream);
            pulseRenderer.write({duration, PulseRenderer::VALUE_PULSE});
        } else {
            m_eof = true;
        }
    }
}

const char* C64TapParser::type() {
    return TYPE;
}
        
const char* C64TapParser::currentStatus() {
    snprintf(m_statusBuffer, 32, "V:%d P:%ld/%ld",
        m_version,
        m_currentPosition,
        m_size);
    return m_statusBuffer;
}

bool C64TapParser::playing() {
    return m_valid && !m_eof;
}

