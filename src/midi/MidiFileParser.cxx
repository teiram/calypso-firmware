#include "MidiFileParser.h"
#include "calypso-debug.h"
#include <cstring>

using namespace calypso;

MidiFileParser::MidiFileParser() {
    resetPlayback();
}

void MidiFileParser::resetPlayback() {
    m_usPerQuarter = DEFAULT_US_PER_QUARTER;
    m_currentTick = 0;
    m_currentTimeUs = 0;
    m_startUs = 0;
    m_started = false;
    m_ended = false;
    m_scheduledValid = false;
    for (uint8_t i = 0; i < m_numTracks; i++) {
        m_tracks[i].cursor = m_tracks[i].start;
        m_tracks[i].tick = 0;
        m_tracks[i].runningStatus = 0;
        m_tracks[i].loaded = false;
        m_tracks[i].ended = false;
    }
}

bool MidiFileParser::readByte(Stream &stream, uint8_t &value) {
    int16_t next = stream.read();
    if (next < 0) {
        return false;
    }
    value = (uint8_t) next;
    return true;
}

bool MidiFileParser::readVlq(Stream &stream, uint32_t &value) {
    value = 0;
    uint8_t byte;
    // A delta time/length is at most four 7-bit groups.
    for (uint8_t i = 0; i < 4; i++) {
        if (!readByte(stream, byte)) {
            return false;
        }
        value = (value << 7) | (byte & 0x7f);
        if (!(byte & 0x80)) {
            return true;
        }
    }
    return false;
}

bool MidiFileParser::insert(Stream &stream) {
    MIDI_DEBUG_LOG(L_DEBUG, "MidiFileParser::insert\n");
    m_numTracks = 0;
    stream.seekSet(0);

    uint8_t header[4];
    if (stream.read(header, 4) != 4 || memcmp(header, "MThd", 4) != 0) {
        MIDI_DEBUG_LOG(L_WARN, "Not a Standard MIDI File\n");
        return false;
    }
    uint8_t lengthBytes[4];
    if (stream.read(lengthBytes, 4) != 4) {
        return false;
    }
    const uint32_t headerLength = ((uint32_t) lengthBytes[0] << 24) | ((uint32_t) lengthBytes[1] << 16) |
                                  ((uint32_t) lengthBytes[2] << 8) | lengthBytes[3];
    uint8_t body[6] = {0, 0, 0, 0, 0, 0};
    const uint32_t bodySize = headerLength < 6 ? headerLength : 6;
    if (stream.read(body, bodySize) != bodySize) {
        return false;
    }
    // Skip any header bytes beyond the six the spec defines.
    stream.seekSet(8 + headerLength);
    const uint16_t format = ((uint16_t) body[0] << 8) | body[1];
    const uint16_t numTracks = ((uint16_t) body[2] << 8) | body[3];
    const uint16_t division = ((uint16_t) body[4] << 8) | body[5];

    if (format > 1) {
        MIDI_DEBUG_LOG(L_WARN, "Unsupported MIDI format %d\n", format);
        return false;
    }
    if (division & 0x8000) {
        MIDI_DEBUG_LOG(L_WARN, "SMPTE MIDI division not supported\n");
        return false;
    }
    m_division = division ? division : 96;

    const uint16_t wanted = numTracks > MAX_TRACKS ? MAX_TRACKS : numTracks;
    for (uint16_t i = 0; i < wanted; i++) {
        // Chunks other than MTrk are legal; step over them.
        while (true) {
            uint8_t chunk[8];
            if (stream.read(chunk, 8) != 8) {
                resetPlayback();
                return m_numTracks > 0;
            }
            const uint32_t chunkLength = ((uint32_t) chunk[4] << 24) | ((uint32_t) chunk[5] << 16) |
                                         ((uint32_t) chunk[6] << 8) | chunk[7];
            if (memcmp(chunk, "MTrk", 4) == 0) {
                Track &track = m_tracks[m_numTracks++];
                track.start = stream.position();
                track.end = track.start + chunkLength;
                track.cursor = track.start;
                track.tick = 0;
                track.runningStatus = 0;
                track.loaded = false;
                track.ended = false;
                // Continue the scan from the next chunk.
                stream.seekSet(track.end);
                break;
            }
            stream.seekCur(chunkLength);
        }
    }

    resetPlayback();
    MIDI_DEBUG_LOG(L_DEBUG, "MIDI format %d, %d tracks, division %d\n",
        format, m_numTracks, m_division);
    return m_numTracks > 0;
}

void MidiFileParser::rewind(Stream &stream) {
    MIDI_DEBUG_LOG(L_DEBUG, "MidiFileParser::rewind\n");
    resetPlayback();
}

void MidiFileParser::start(uint64_t nowUs) {
    m_startUs = nowUs;
    m_started = true;
    m_ended = false;
}

// Read one event from a track into its pending slot. Returns false when the
// track is exhausted. Running status is resolved here but the buffered message
// always carries an explicit status byte.
bool MidiFileParser::loadNext(Stream &stream, Track &track) {
    if (track.cursor >= track.end || !stream.seekSet(track.cursor)) {
        track.ended = true;
        track.loaded = false;
        return false;
    }

    uint32_t delta;
    if (!readVlq(stream, delta)) {
        track.ended = true;
        track.loaded = false;
        return false;
    }
    track.tick += delta;

    int16_t statusOrData = stream.read();
    if (statusOrData < 0) {
        track.ended = true;
        track.loaded = false;
        return false;
    }

    uint8_t status;
    if (statusOrData & 0x80) {
        status = (uint8_t) statusOrData;
        if (status >= 0xf0) {
            if (status == 0xff) {
                uint8_t metaType;
                uint32_t metaLength;
                if (!readByte(stream, metaType) || !readVlq(stream, metaLength)) {
                    track.ended = true;
                    track.loaded = false;
                    return false;
                }
                if (metaType == 0x51 && metaLength == 3) {
                    uint8_t a, b, c;
                    if (!readByte(stream, a) || !readByte(stream, b) || !readByte(stream, c)) {
                        track.ended = true;
                        track.loaded = false;
                        return false;
                    }
                    track.pendingType = EVENT_TEMPO;
                    track.pendingTempo = ((uint32_t) a << 16) | ((uint32_t) b << 8) | c;
                    track.pendingLength = 0;
                } else {
                    stream.seekCur(metaLength);
                    track.pendingType = EVENT_SKIP;
                    track.pendingLength = 0;
                }
            } else {
                // Sysex (0xf0/0xf7): read through, and per the spec it cancels
                // the running status on this track.
                uint32_t sysexLength;
                if (!readVlq(stream, sysexLength)) {
                    track.ended = true;
                    track.loaded = false;
                    return false;
                }
                stream.seekCur(sysexLength);
                track.runningStatus = 0;
                track.pendingType = EVENT_SKIP;
                track.pendingLength = 0;
            }
            track.pendingTick = track.tick;
            track.loaded = true;
            track.cursor = stream.position();
            return true;
        }
        track.runningStatus = status;
    } else {
        // Running status: the byte read is the message's first data byte.
        status = track.runningStatus;
        if (status == 0) {
            track.ended = true;
            track.loaded = false;
            return false;
        }
        stream.seekCur(-1);
    }

    const uint8_t kind = status & 0xf0;
    const uint8_t dataBytes = (kind == 0xc0 || kind == 0xd0) ? 1 : 2;
    uint8_t data[2] = {0, 0};
    for (uint8_t i = 0; i < dataBytes; i++) {
        if (!readByte(stream, data[i])) {
            track.ended = true;
            track.loaded = false;
            return false;
        }
    }

    track.pendingTick = track.tick;
    if (kind == 0x80 || kind == 0x90 || kind == 0xb0 || kind == 0xc0 || kind == 0xe0) {
        track.pendingPayload[0] = status;
        track.pendingPayload[1] = data[0];
        if (dataBytes > 1) {
            track.pendingPayload[2] = data[1];
        }
        track.pendingLength = 1 + dataBytes;
        track.pendingType = EVENT_MESSAGE;
    } else {
        // Polyphonic aftertouch / channel pressure: parsed, not forwarded.
        track.pendingType = EVENT_SKIP;
        track.pendingLength = 0;
    }
    track.loaded = true;
    track.cursor = stream.position();
    return true;
}

bool MidiFileParser::step(uint64_t nowUs, Stream &stream, MidiSink &sink) {
    if (!m_started || m_ended) {
        return false;
    }
    const uint64_t elapsed = nowUs - m_startUs;

    while (true) {
        uint32_t minTick = 0xffffffffu;
        bool anyPending = false;
        for (uint8_t i = 0; i < m_numTracks; i++) {
            Track &track = m_tracks[i];
            if (!track.loaded && !track.ended) {
                loadNext(stream, track);
            }
            if (track.loaded) {
                anyPending = true;
                if (track.pendingTick < minTick) {
                    minTick = track.pendingTick;
                }
            }
        }
        if (!anyPending) {
            m_ended = true;
            return false;
        }

        uint64_t targetUs;
        if (minTick <= m_currentTick) {
            targetUs = m_currentTimeUs;
        } else if (m_scheduledValid && m_scheduledTick == minTick) {
            targetUs = m_scheduledTimeUs;
        } else {
            targetUs = m_currentTimeUs +
                ((uint64_t) (minTick - m_currentTick) * m_usPerQuarter) / m_division;
            m_scheduledTick = minTick;
            m_scheduledTimeUs = targetUs;
            m_scheduledValid = true;
        }
        if (elapsed < targetUs) {
            return true;
        }

        m_currentTimeUs = targetUs;
        m_currentTick = minTick;
        for (uint8_t i = 0; i < m_numTracks; i++) {
            Track &track = m_tracks[i];
            if (!track.loaded || track.pendingTick != minTick) {
                continue;
            }
            if (track.pendingType == EVENT_TEMPO) {
                // Applies to every event after this tick.
                m_usPerQuarter = track.pendingTempo;
            } else if (track.pendingType == EVENT_MESSAGE) {
                for (uint8_t b = 0; b < track.pendingLength; b++) {
                    sink.midiByte(track.pendingPayload[b]);
                }
            }
            track.loaded = false;
        }
    }
}

const char *MidiFileParser::type() const {
    return TYPE;
}

const char *MidiFileParser::currentStatus() {
    snprintf(m_statusBuffer, sizeof(m_statusBuffer), "Tracks:%u Tick:%lu",
        (unsigned) m_numTracks, (unsigned long) m_currentTick);
    return m_statusBuffer;
}

bool MidiFileParser::ended() const {
    return m_ended;
}

uint8_t MidiFileParser::numTracks() const {
    return m_numTracks;
}

uint8_t MidiFileParser::progress() const {
    if (m_ended) {
        return 100;
    }
    uint64_t done = 0;
    uint64_t total = 0;
    for (uint8_t i = 0; i < m_numTracks; i++) {
        const Track &track = m_tracks[i];
        total += track.end - track.start;
        // A track stops reading at its end, so a cursor that overshot is the
        // whole chunk; tracks that have not started contribute nothing.
        const uint32_t cursor = track.cursor < track.end ? track.cursor : track.end;
        if (cursor > track.start) {
            done += cursor - track.start;
        }
    }
    return total ? (uint8_t) ((done * 100) / total) : 0;
}
