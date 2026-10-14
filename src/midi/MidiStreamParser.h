#ifndef MIDI_STREAM_PARSER_H
#define MIDI_STREAM_PARSER_H

#include <cinttypes>
#include "Player.h"

// Turns the raw MIDI byte stream the Calypso mist/FPGA link delivers into Player
// calls. This is not the USB-MIDI path: a serial stream uses running status (a
// channel message may be followed by data bytes alone) and realtime bytes may
// appear between any two bytes of a message. Only the channel messages the
// players understand are forwarded:
//   0x8n note off         0x9n note on (velocity 0 is a note off)
//   0xBn control change   0xCn program change   0xEn pitch bend
// Control change forwards CC7 (channel volume) and CC10 (pan); 0xAn polyphonic
// aftertouch and 0xDn channel pressure are parsed and dropped.
class MidiStreamParser {
public:
    explicit MidiStreamParser(Player &player);
    void accept(uint8_t midiByte);
private:
    static constexpr uint8_t STATUS_NONE = 0xff;

    Player &m_player;
    uint8_t m_status = STATUS_NONE;   // last channel status, for running status
    uint8_t m_data[2] = {0, 0};
    uint8_t m_dataCount = 0;
    uint8_t m_expected = 0;

    void reset();
    void dispatch();
};

#endif /* MIDI_STREAM_PARSER_H */
