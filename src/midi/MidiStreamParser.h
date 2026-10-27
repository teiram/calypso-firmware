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
// Control change forwards CC7 (channel volume) and CC10 (pan), CC120 (All Sound
// Off) to Player::allSoundOff() and CC123 (All Notes Off) to
// Player::allNotesOff(), and tracks the RPN/NRPN selection (CC101/CC100 vs
// CC99/CC98, with the null RPN 127/127 deselecting) so that data entry
// (CC6/CC38) on RPN 0 -- the pitch bend sensitivity -- reaches
// Player::setPitchBendRange(), while data entry meant for an NRPN does not;
// 0xAn polyphonic aftertouch and 0xDn channel pressure are parsed and dropped.
class MidiStreamParser {
public:
    explicit MidiStreamParser(Player &player);
    void accept(uint8_t midiByte);
    // Songs end without resetting the RPN machinery: a fresh file must not
    // inherit the previous one's selection (its data entries would then be
    // misrouted) or data-entry defaults. Also clears any partial message.
    void resetChannelState();
private:
    static constexpr uint8_t STATUS_NONE = 0xff;
    static constexpr uint8_t RPN_NONE = 0x7f;

    Player &m_player;
    uint8_t m_status = STATUS_NONE;   // last channel status, for running status
    uint8_t m_data[2] = {0, 0};
    uint8_t m_dataCount = 0;
    uint8_t m_expected = 0;
    // Selected RPN per channel, deselected (127/127) until the stream picks one
    // (initialised in the constructor). m_nrpnSelected tracks whether an NRPN
    // was selected more recently: data entry then belongs to the NRPN, not the
    // RPN (a GS vibrato-rate write must not become a bend sensitivity).
    uint8_t m_rpnMsb[16];
    uint8_t m_rpnLsb[16];
    bool m_nrpnSelected[16];
    // Last data entry halves per channel; the GM default bend range is +/-2.
    uint8_t m_dataEntryMsb[16];
    uint8_t m_dataEntryLsb[16];

    void reset();
    void dispatch();
};

#endif /* MIDI_STREAM_PARSER_H */
