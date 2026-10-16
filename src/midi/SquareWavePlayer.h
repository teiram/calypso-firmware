#ifndef SQUARE_WAVE_PLAYER_H
#define SQUARE_WAVE_PLAYER_H

#include <ctime>
#include "Player.h"
#include "AudioTarget.h"

class SquareWavePlayer: public Player {
public:
    static constexpr size_t NUM_OSC = 128;
    static constexpr size_t NUM_CHN = 16;
    static constexpr uint32_t DEFAULT_SAMPLE_FREQ = 24000;
    // The square engine is indifferent to the block size, so it asks for the
    // long blocks the firmware used before the wavetable engine arrived.
    static constexpr uint16_t AUDIO_BLOCK_SAMPLES = 256;
    static constexpr uint16_t AUDIO_POOL_BUFFERS = 8;

    typedef struct {
        uint32_t count_wrap;
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
    static const uint32_t COUNT_INC;
    SquareWavePlayer(AudioTarget &audioTarget);
    virtual bool init();
    virtual void playNote(uint8_t channel, uint8_t pitch, uint8_t velocity);
    virtual void stopNote(uint8_t channel, uint8_t pitch);
    virtual void allNotesOff();
    virtual void synthTask();
    virtual void showStats();
    virtual uint16_t audioBufferCount() const;
    virtual uint16_t audioBufferSamples() const;
private:
    static constexpr double OCTAVE_FREQ_RATIO = 2.0;
    static constexpr uint8_t NOTES_PER_OCTAVE = 12;
    static constexpr double A4_FREQ = 440.0;
    static constexpr uint8_t A4_NOTE_NUMBER = 69;
    static constexpr uint8_t COUNT_HEADROOM_BITS = 8;
    static constexpr uint8_t VOL_BITS = 8;

    OscStatus m_oscStatuses[NUM_OSC];
    MidiStatus m_midiStatus;
    AudioTarget& m_audioTarget;
    uint8_t m_skipCount = 0;
    uint8_t m_msgCount = 0;
    uint64_t m_timestampActiveSensing;

    void oscInit(const uint32_t sampleFrequency);
    void stateInit();
    void ledInit(const uint8_t gpioPinActivityIndicator);
    void addToOscStatus(const uint8_t pitch, const int8_t deltaVelocity);
};

#endif /* SQUARE_WAVE_PLAYER_H */
