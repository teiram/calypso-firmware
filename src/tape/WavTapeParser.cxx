#include "WavTapeParser.h"
#include "calypso-debug.h"

using namespace calypso;

WavTapeParser::WavTapeParser():
    m_state(WAV_IDLE),
    m_currentPosition(0),
    m_bitsPerSample(0),
    m_sampleRate(0),
    m_streamSize(0) {    
}

TapeConfiguration WavTapeParser::configuration() {
    return CONFIGURATION;
}

const char *WavTapeParser::type() {
    return TYPE;
}

bool WavTapeParser::insert(Stream& stream) {
    TAPE_DEBUG_LOG(L_DEBUG, "WavTapeParser::insert\n");
    stream.seekSet(22);
    stream.read(&m_channelCount, 2);
    stream.read(&m_sampleRate, 4);
    stream.seekSet(34);
    stream.read(&m_bitsPerSample, 2);
    stream.seekSet(40);
    stream.read(&m_streamSize, 4);
    if ((m_bitsPerSample == 8 || m_bitsPerSample == 16) && m_sampleRate != 0) {
        m_state = WAV_INITIALIZED;
    }
    TAPE_DEBUG_LOG(L_DEBUG, "Wav initialized with channel count %d, sampleRate %d,\n  bitsPerSample: %d, streamSize: %d\n",
        m_channelCount, m_sampleRate, m_bitsPerSample, m_streamSize);
    return m_state == WAV_INITIALIZED;
}

const char *WavTapeParser::currentStatus() {
        snprintf(m_statusBuffer, 32, "S:%d,C:%d,R:%ld,P:%ld",
        m_state,
        m_channelCount,
        m_sampleRate,
        m_currentPosition);

    return m_statusBuffer;
}

bool WavTapeParser::playing() {
    return m_state == WAV_INITIALIZED;
}

void WavTapeParser::rewind(Stream &stream) {
    stream.seekSet(HEADER_SIZE);
    m_currentPosition = stream.position();
    m_state = WAV_INITIALIZED;
}

bool WavTapeParser::needsAttention() {
    return m_state == WAV_INITIALIZED;
}

uint32_t WavTapeParser::getNextPulseLength(Stream &stream) {
    uint32_t counter = 0;
    bool eof = false;
    if (m_bitsPerSample == 8) {
        // 1 byte per sample, unsigned
        uint8_t initial_value = 0x80;
        // Get rid of initial silence
        while (initial_value == 0x80 && !eof) {
            int16_t v = stream.read();
            if (v != -1) {
                initial_value = v & 0xff;
            } else {
                eof = true;
            }
        }
        uint8_t value = initial_value;
        while (value == initial_value && !eof) {
            counter++;
            int16_t v = stream.read();
            if (v != -1) {
                value = v & 0xff;
            } else {
                eof = true;
            }
        }
        if (eof) {
            m_state = WAV_EOF;
        } else {
            stream.seekCur(-1);
        }
    } else {
        int8_t initial_value = 0;
        while (initial_value == 0 && !eof) {
            int16_t buffer;
            if (stream.read(&buffer, 2)) {
                if (buffer < -255 || buffer > 255) {
                    initial_value = buffer > 255 ? 1 : -1;
                }
            } else {
                eof = true;
            }
        }
        int8_t value = initial_value;
        while (value == initial_value && !eof) {
            int16_t buffer;
            counter++;
            if (stream.read(&buffer, 2)) {
                value = buffer > 0 ? 1 : -1;
            } else {
                eof = true;
            }
        }
        if (eof) {
            m_state = WAV_EOF;
        } else {
            stream.seekCur(-2);
        }
    }
    m_currentPosition = stream.position();
    // From counter to microseconds
    uint32_t usecs =  (1000000 * counter) / m_sampleRate;
    return usecs;
}

void WavTapeParser::renderStep(PulseRenderer &pulseRenderer, Stream &stream) {
    while (!pulseRenderer.full() && m_state == WAV_INITIALIZED) {
        if (m_currentPosition < m_streamSize + HEADER_SIZE) {
            //Get count of next samples on the same level
            uint32_t usecs = getNextPulseLength(stream);
            //This will be half a pulse for the renderer
            if (usecs != 0) {
                pulseRenderer.write({.value = usecs, .flags = 0});
            }
        } else {
            m_state = WAV_EOF;
        }
    }
}