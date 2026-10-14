#include "FixedReverb.h"

FixedReverb::FixedReverb(int32_t dampingFract15, int32_t wetFract16):
    m_damping(dampingFract15),
    m_feedback(fract15(0.84)),
    m_allpassGain(fract15(0.5)),
    m_wet(wetFract16),
    m_lastOutL(0),
    m_lastOutR(0) {
    for (uint16_t i = 0; i < 8; i++) {
        m_combs[i].index = 0;
        m_combs[i].filterStore = 0;
        for (uint16_t j = 0; j < MAX_COMB_LENGTH; j++) {
            m_combs[i].buffer[j] = 0;
        }
    }
    for (uint16_t i = 0; i < 2; i++) {
        m_allpasses[i].index = 0;
        for (uint16_t j = 0; j < MAX_ALLPASS_LENGTH; j++) {
            m_allpasses[i].buffer[j] = 0;
        }
    }
}

// Q15 multiply: (a * b) >> 15. Both operands stay far below full scale for the
// values that matter (feedback/damping/allpass < 1, loop levels < ~200k), so the
// 32 bit product is never close to overflowing.
static inline int32_t fpmul(int32_t a, int32_t b) {
    return (a * b) >> 15;
}

// Q16 multiply used for the output trim.
static inline int32_t fp16mul(int32_t a, int32_t b) {
    return (a * b) >> 16;
}

void FixedReverb::process(int32_t dryIn, int32_t &wetOutL, int32_t &wetOutR) {
    // Half the mixer level enters the loop. With feedback 0.84 a comb's store
    // asymptotes at about 6x its input, so even a full scale dry sample (32k)
    // only reaches ~100k inside a line - well inside int32.
    int32_t x = dryIn >> 1;

    // Two series allpasses thicken the early reflections so the combs do not
    // repeat one clean echo each. Lattice form: y = b - g*x; b = x + g*b.
    for (uint16_t i = 0; i < 2; i++) {
        Allpass &ap = m_allpasses[i];
        const int32_t b = ap.buffer[ap.index];
        const int32_t y = b - fpmul(m_allpassGain, x);
        ap.buffer[ap.index] = x + fpmul(m_allpassGain, b);
        if (++ap.index >= ALLPASS_LENGTHS[i]) {
            ap.index = 0;
        }
        x = y;
    }

    // Each comb loops input + damped feedback. The damped tap is what goes
    // back in the loop; the wet output taps the raw delayed sample, exactly as
    // the classic design does, so a burst comes out through the echoes at full
    // amplitude while the damping one pole (on the stored value) only shapes the
    // loop's high end. Even combs feed the left wet, odd the right: the two
    // channel patterns are built from different delay lengths, so they
    // decorrelate and the plate sounds wide.
    int32_t outL = 0;
    int32_t outR = 0;
    for (uint16_t i = 0; i < 8; i++) {
        Comb &cb = m_combs[i];
        const int32_t raw = cb.buffer[cb.index];
        cb.filterStore += fpmul(m_damping, raw - cb.filterStore);
        const int32_t damped = cb.filterStore;
        const int32_t next = x + fpmul(m_feedback, damped);
        cb.buffer[cb.index] = next < -(1 << 30) ? -(1 << 30) : (next > (1 << 30) ? (1 << 30) : next);
        if (++cb.index >= COMB_LENGTHS[i]) {
            cb.index = 0;
        }
        if ((i & 1) == 0) {
            outL += raw;
        } else {
            outR += raw;
        }
    }

    // Four combs each side. Each comb runs at loop gain 0.84, so four of them
    // sum to roughly 4 * 6x = 24x the allpassed feed; /8 brings the summed wet
    // close to the dry level, and m_wet is the final trim the user hears.
    m_lastOutL = fp16mul(m_wet, outL >> 3);
    m_lastOutR = fp16mul(m_wet, outR >> 3);
    wetOutL = m_lastOutL;
    wetOutR = m_lastOutR;
}