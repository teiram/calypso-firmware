#ifndef FIXEDREVERB_H
#define FIXEDREVERB_H

#include <cinttypes>

// Fixed point plate reverb used to open up the dry wavetable mix. This is the
// classic Freeverb topology - two input allpass ladders driving eight parallel
// combs, each with a one pole low pass in its feedback path - done in Q15 so it
// runs inside the synth loop without a single division. The even combs feed the
// left wet output and the odd combs the right, so the two echo patterns are
// decorrelated and the reverb sounds wide instead of pinned to the centre.
//
// Delay lines are int32, not int16: a single comb's loop level runs up to about
// input/(1-feedback) = 6.25x the feed for a sustained tone, which overflows a
// 16 bit line on anything but the softest notes. Memory: 8 combs + 2 allpass,
// lengths scaled from Freeverb's 44.1 kHz values to the 24 kHz output rate,
// ~31 KiB of int32 buffers. Feedback 0.84 gives a 1.0..1.4 s tail; the damping
// coefficient darkens it.
class FixedReverb {
public:
    // dampingFract15: one pole low pass applied inside each comb's feedback
    // loop (0..1, ~0.28 is a pleasant plate); wetFract16: output level.
    explicit FixedReverb(int32_t dampingFract15 = 0x23d7, int32_t wetFract16 = 0x8000);

    // Feed one mono sample in, get decorrelated left/right wet levels back.
    // dryIn is whatever the mixer summed (int32); it is scaled on entry so the
    // comb feedback loop can never run away.
    void process(int32_t dryIn, int32_t &wetOutL, int32_t &wetOutR);

    // Empty every delay line. The tail is previous audio by definition, so a
    // transport stop clears it: otherwise the next thing played starts inside
    // whatever was still ringing.
    void reset();

private:
    static constexpr int32_t fract15(double v) { return (int32_t) (v * 32768.0); }
    static constexpr int32_t fract16(double v) { return (int32_t) (v * 65536.0); }

    // longest delay line per section; keeps each buffer's row a compile-time size
    static constexpr uint16_t MAX_COMB_LENGTH = 900;
    static constexpr uint16_t MAX_ALLPASS_LENGTH = 310;

    static constexpr uint16_t COMB_LENGTHS[8] = {
        607, 646, 695, 738, 774, 811, 839, 879
    };
    static constexpr uint16_t ALLPASS_LENGTHS[2] = { 122, 302 };

    struct Comb {
        int32_t buffer[MAX_COMB_LENGTH];
        uint16_t index;
        int32_t filterStore;
    };
    struct Allpass {
        int32_t buffer[MAX_ALLPASS_LENGTH];
        uint16_t index;
    };

    Comb m_combs[8];
    Allpass m_allpasses[2];
    int32_t m_damping;    // Q15, 0..1
    int32_t m_feedback;   // Q15, < 1 keeps every loop bounded
    int32_t m_allpassGain; // Q15
    int32_t m_wet;        // Q16 output trim
    int32_t m_lastOutL;
    int32_t m_lastOutR;
};

#endif /* FIXEDREVERB_H */