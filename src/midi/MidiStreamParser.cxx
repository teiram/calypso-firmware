#include "MidiStreamParser.h"

MidiStreamParser::MidiStreamParser(Player &player): m_player(player) {
    resetChannelState();
}

void MidiStreamParser::resetChannelState() {
    reset();
    for (uint8_t i = 0; i < 16; i++) {
        m_rpnMsb[i] = RPN_NONE;
        m_rpnLsb[i] = RPN_NONE;
        m_nrpnSelected[i] = false;
        m_dataEntryMsb[i] = 2; // the GM default bend sensitivity is +/-2 semitones
        m_dataEntryLsb[i] = 0;
    }
}

void MidiStreamParser::reset() {
    m_status = STATUS_NONE;
    m_dataCount = 0;
    m_expected = 0;
}

void MidiStreamParser::accept(uint8_t midiByte) {
    if (midiByte >= 0xf8) {
        // Realtime (clock, active sensing, reset). It is legal between any two
        // bytes of a message and belongs to no channel message, so it neither
        // advances nor clears the parser.
        return;
    }
    if (midiByte & 0x80) {
        // A status byte starts a new message; any partial one is discarded.
        m_dataCount = 0;
        if (midiByte >= 0xf0) {
            // System common (sysex, song position, tune request): not supported.
            // Dropping running status here is what the spec asks for.
            reset();
            return;
        }
        m_status = midiByte;
        const uint8_t kind = midiByte & 0xf0;
        // Program change (0xCn) and channel pressure (0xDn) carry one data byte.
        m_expected = (kind == 0xc0 || kind == 0xd0) ? 1 : 2;
        return;
    }

    if (m_status == STATUS_NONE) {
        return; // a data byte with no channel message in progress
    }
    m_data[m_dataCount++] = midiByte;
    if (m_dataCount >= m_expected) {
        dispatch();
        m_dataCount = 0; // stay on m_status for running status
    }
}

void MidiStreamParser::dispatch() {
    const uint8_t channel = m_status & 0x0f;
    switch (m_status & 0xf0) {
        case 0x80:
            m_player.stopNote(channel, m_data[0] & 0x7f);
            break;
        case 0x90: {
            const uint8_t pitch = m_data[0] & 0x7f;
            const uint8_t velocity = m_data[1] & 0x7f;
            if (velocity == 0) {
                m_player.stopNote(channel, pitch);
            } else {
                m_player.playNote(channel, pitch, velocity);
            }
            break;
        }
        case 0xb0: {
            const uint8_t controller = m_data[0] & 0x7f;
            const uint8_t value = m_data[1] & 0x7f;
            if (controller == 7) {
                m_player.setVolume(channel, value);
            } else if (controller == 10) {
                m_player.setPan(channel, value);
            } else if (controller == 101) {
                // RPN select MSB; (127, 127) is the null RPN and deselects.
                m_rpnMsb[channel] = value;
                m_nrpnSelected[channel] = false;
            } else if (controller == 100) {
                m_rpnLsb[channel] = value;
                m_nrpnSelected[channel] = false;
            } else if (controller == 99 || controller == 98) {
                // NRPN select: data entry now targets the NRPN, not the RPN.
                m_nrpnSelected[channel] = true;
            } else if (controller == 6 || controller == 38) {
                // Data entry for the selected RPN. Only RPN 0x0000 (pitch bend
                // sensitivity) is understood; the two halves can arrive in either
                // order, so both are kept and the range is reissued on a change.
                if (!m_nrpnSelected[channel] &&
                    m_rpnMsb[channel] == 0 && m_rpnLsb[channel] == 0) {
                    if (controller == 6) {
                        m_dataEntryMsb[channel] = value;
                    } else {
                        m_dataEntryLsb[channel] = value;
                    }
                    m_player.setPitchBendRange(channel,
                        m_dataEntryMsb[channel], m_dataEntryLsb[channel]);
                }
            } else if (controller == 120) {
                // All Sound Off: immediate silence, no release phase.
                m_player.allSoundOff();
            } else if (controller == 123) {
                // All Notes Off: each key is let go, as a note-off per key would.
                // This is also the panic the idle watchdog raises when no message
                // comes at all.
                m_player.allNotesOff();
            }
            break;
        }
        case 0xc0:
            m_player.changeProgram(channel, m_data[0] & 0x7f);
            break;
        case 0xe0: {
            const uint16_t bending = ((uint16_t) (m_data[1] & 0x7f) << 7) | (m_data[0] & 0x7f);
            m_player.setPitchBending(channel, bending);
            break;
        }
        default:
            break; // polyphonic aftertouch / channel pressure: parsed, dropped
    }
}
