#ifndef TONE_GENERATOR_H
#define TONE_GENERATOR_H

#include <cinttypes>

enum EnvelopeState {
    ENV_IDLE,
    ENV_DELAY,
    ENV_ATTACK,
    ENV_HOLD,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
};

// Instrument index now directly maps to MIDI program number (0-127)
typedef uint8_t InstrumentIndex;

struct ToneGenerator {

    uint32_t tonePhase;                 // where we are playing in an instrument sample
    uint32_t toneIncrement;             // increment from one sample to another
    int32_t volume;                     // midi volume from 1..127 code (2^16 fraction)
    uint32_t sampleEndingIndex;         // last frame of a finite sample voice: the one-shot's
                                        // end or the loop's wrap point (uint32, not uint16, so a
                                        // sample may run past 65535 frames)
    uint32_t sampleLoopStart;           // wrap target of a looped sample voice
    uint16_t startSerial;               // note issue order, used to pick a voice to steal
    uint8_t instrumentIndex;    // the instrument we're playing: I_PIANO, etc.
    bool playing;                       // is this channel playing?
    bool percussion;                    // is it a percussion instrument?
    bool sampleVoice;                   // a recorded sample (pitched and/or looped), not a cycle
    bool sampleLoops;                   // wrap at sampleEndingIndex instead of running off the end
    EnvelopeState envelopeState;        // envelope state variables:
    int32_t envelopeMultiplier;         // amplitude multiplier  as fraction * 2^16
    int32_t envelopeIncrement;          // amplitude increment, as fraction * 2^16
    int32_t envelopeRate;               // per sample decay proportion, fraction * 2^20
    int32_t envelopeCarry;              // remainder of the last decay step, so tiny levels decay
    int32_t envelopeCount;              // duration count for this state, as number of samples
    uint8_t midiChannel;                // the midi channel playing the note
    int32_t panL;                       // left gain, equal-power fract16 pair with panR
    int32_t panR;                       // right gain; index 0..PAN_STEPS picks the pair
    uint8_t tableShift;                 // log2 of the instrument table length: 8 for the
                                        // 256 point cycles; synthTask derives its index and
                                        // fraction bit shifts from this
    int32_t envelopeAttack;             // per note-on copy of the envelope stages, so a voice
    int32_t envelopeHold;               // keeps the shape it started with even if the channel's
    int32_t envelopeDecay;              // program changes underneath it
    int32_t envelopeRelease;
    int32_t envelopeSustainLevel;       // sustain level, fraction * 2^16
    const int16_t* waveform;            // pointer to the waveform sample array
                                        // with 2^tableShift points for instruments, up to 65535 for percussion
};

#endif /* TONE_GENERATOR_H */
