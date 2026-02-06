#ifndef MIDI_STATE_MACHINE_H
#define MIDI_STATE_MACHINE_H

#include <ctime>
#include <cinttypes>

class MIDIStateMachine {
public:
    static constexpr size_t NUM_OSC = 128;
    static constexpr size_t NUM_CHN = 16;
    typedef struct {
        uint32_t countWrap;
        uint32_t count;
        uint16_t velocity;
        int16_t elongation;
    } OscStatus;
    typedef struct {
        uint8_t velocity;
    } NoteStatus;
    typedef struct {
        NoteStatus noteStatus[NUM_OSC];
    } ChannelStatus;
    typedef struct {
        ChannelStatus channelStatus[NUM_CHN];
    } MidiStatus;

    MIDIStateMachine();
    bool init(const uint32_t sampleFrequency);
    OscStatus* oscStatuses();
    void addMidiByte(uint8_t midiByte);
private:
    static constexpr double OCTAVE_FREQ_RATIO = 2.0;
    static constexpr uint8_t NOTES_PER_OCTAVE = 12;
    static constexpr double A4_FREQ = 440.0;
    static constexpr uint8_t A4_NOTE_NUMBER = 69;
    static constexpr uint8_t COUNT_HEADROOM_BITS = 8;
    typedef enum {
        WAIT_CHANNEL,
        WAIT_PITCH,
        WAIT_VELOCITY
    } ParserState;
    typedef struct {
        bool noteOn;
        uint8_t channel;
        uint8_t pitch;
        uint8_t velocity;
    } MidiEvent;
    OscStatus m_oscStatuses[NUM_OSC];
    MidiStatus m_midiStatus;
    uint8_t m_skipCount = 0;
    uint8_t m_msgCount = 0;
    uint64_t m_timestampActingSensing;
    ParserState m_parserState;
    MidiEvent m_currentMidiEvent;
    void oscInit(const uint32_t sampleFrequency);
    void stateInit();
    void reset();
    void processEvent(const MidiEvent &event);
    void addNote(const uint8_t pitch, const int8_t delta_velocity);

    public:
    static constexpr uint32_t COUNT_INC = ((uint32_t) 1) << COUNT_HEADROOM_BITS;;
};

#endif /* MIDI_STATE_MACHINE_H */

