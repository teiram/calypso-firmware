#ifndef WAVETABLEPLAYER_H
#define WAVETABLEPLAYER_H

#include <cinttypes>
#include "Player.h"
#include "PlayerUtil.h"
#include "AudioTarget.h"
#include "ToneGenerator.h"
#include "FixedReverb.h"
#include <cmath>

// Gain tables are applied to 16 bit voice samples on the way in, so an entry above unity would
// let a voice exceed full scale before the bus limiter ever saw it.
constexpr bool withinUnity(const int32_t *table, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (table[i] < 0 || table[i] > 0x10000) {
            return false;
        }
    }
    return true;
}

// Per-key cycle rows: a program's palette is a list of 256-point cycles, each
// covering MIDI keys up to and including keyHigh[i].  Generated from a GM
// soundfont's multisample layout (tools/make_flr3_cycles.py), so a low piano
// string and a high one carry different harmonic content.  rowCount == 0 means
// the font gave us nothing measurable for that program and the legacy
// single-cycle waveform stands in.
struct waveform_rows_t {
    const int16_t *const *cycles;
    const uint8_t *keyHigh;
    uint8_t rowCount;
};

// An instrument-driven sample voice (tools/make_fx_samples.py): a whole recorded
// sample the engine plays instead of a cycle, for the GM programs whose content
// a 256-point cycle cannot represent (a gunshot, applause, a seashore).  It is
// read with the percussion reader's 16-bit index, but on the melodic bus, so a
// voice is panned, velocity-scaled and pitch-bent like any other note.
//   root == 0     play at the recording's own rate (the unity fast path: one
//                 sample per frame, no interpolation, so no aliasing); a
//                 non-zero root transposes the sample to the note.
//   loops         wrap at loopEnd and release on note-off.  A one-shot runs to
//                 loopEnd (its last frame) and retires; a note-off only trims.
struct voice_sample_t {
    const int16_t *samples;   // nullptr: this program is a cycle voice
    uint32_t length;          // frames in samples[]
    uint32_t loopStart;       // wrap target for a loop voice, else 0
    uint32_t loopEnd;         // last frame played: the wrap point, or length-1
    uint8_t root;             // 0: fixed pitch; else the sample's own MIDI root
    bool loops;
};

class WaveTablePlayer: public Player {
private:
    static constexpr uint8_t MAX_TGENS = 64;
    static constexpr uint8_t MIDI_CHANNELS = 16;
    static constexpr uint8_t MIN_NOTE = 21;
    static constexpr uint8_t MAX_NOTE = 108;
    static constexpr uint8_t NUM_NOTES = MAX_NOTE - MIN_NOTE + 1;
    static constexpr uint32_t INT_MAX = 0x7FFFFFFF;
    static constexpr uint8_t NO_CHANNEL = 0xff;
    static constexpr uint8_t NO_PITCH = 0xff;

    // well-tempered MIDI note frequencies, based on the 12th root of 2.
    // note frequencies * 4096
    /* 21..108 */ 
    static constexpr uint32_t FREQ4096[NUM_NOTES] = {   
        112640, 119338, 126434, 133952, 141918, 150356, 159297,
        168769, 178805, 189437, 200702, 212636, 225280, 238676, 252868,
        267905, 283835, 300713, 318594, 337539, 357610, 378874, 401403,
        425272, 450560, 477352, 505737, 535809, 567670, 601425, 637188,
        675077, 715219, 757749, 802807, 850544, 901120, 954703, 1011473,
        1071618, 1135340, 1202851, 1274376, 1350154, 1430439, 1515497,
        1605613, 1701088, 1802240, 1909407, 2022946, 2143237, 2270680,
        2405702, 2548752, 2700309, 2860878, 3030994, 3211227, 3402176,
        3604480, 3818814, 4045892, 4286473, 4541360, 4811404, 5097505,
        5400618, 5721755, 6061989, 6422453, 6804352, 7208960, 7637627,
        8091784, 8572947, 9082720, 9622807, 10195009, 10801236, 11443511,
        12123977, 12844906, 13608704, 14417920, 15275254, 16183568,
        17145893
    };

    // Per-bus gain as a function of the number of sounding voices on that bus.
    // Voices are largely uncorrelated, so their sum grows as sqrt(n): dividing by sqrt(n)
    // keeps the bus level constant as voices come and go, without the aggressive ducking
    // that 1/n caused (a lone drum used to be cut to 1/n by unrelated sustained pads).
    // Indexed by the *current* sample's voice count: index 0 is idle, index MAX_TGENS must exist.
    static constexpr int32_t VOICE_ATTENUATION[MAX_TGENS + 1] = {
        PlayerUtil::fract16(1.0000), PlayerUtil::fract16(1.0000), PlayerUtil::fract16(0.7071), PlayerUtil::fract16(0.5774), PlayerUtil::fract16(0.5000),
        PlayerUtil::fract16(0.4472), PlayerUtil::fract16(0.4082), PlayerUtil::fract16(0.3780), PlayerUtil::fract16(0.3536), PlayerUtil::fract16(0.3333),
        PlayerUtil::fract16(0.3162), PlayerUtil::fract16(0.3015), PlayerUtil::fract16(0.2887), PlayerUtil::fract16(0.2774), PlayerUtil::fract16(0.2673),
        PlayerUtil::fract16(0.2582), PlayerUtil::fract16(0.2500), PlayerUtil::fract16(0.2425), PlayerUtil::fract16(0.2357), PlayerUtil::fract16(0.2294),
        PlayerUtil::fract16(0.2236), PlayerUtil::fract16(0.2182), PlayerUtil::fract16(0.2132), PlayerUtil::fract16(0.2085), PlayerUtil::fract16(0.2041),
        PlayerUtil::fract16(0.2000), PlayerUtil::fract16(0.1961), PlayerUtil::fract16(0.1925), PlayerUtil::fract16(0.1890), PlayerUtil::fract16(0.1857),
        PlayerUtil::fract16(0.1826), PlayerUtil::fract16(0.1796), PlayerUtil::fract16(0.1768), PlayerUtil::fract16(0.1741), PlayerUtil::fract16(0.1715),
        PlayerUtil::fract16(0.1690), PlayerUtil::fract16(0.1667), PlayerUtil::fract16(0.1644), PlayerUtil::fract16(0.1622), PlayerUtil::fract16(0.1601),
        PlayerUtil::fract16(0.1581), PlayerUtil::fract16(0.1562), PlayerUtil::fract16(0.1543), PlayerUtil::fract16(0.1525), PlayerUtil::fract16(0.1508),
        PlayerUtil::fract16(0.1491), PlayerUtil::fract16(0.1474), PlayerUtil::fract16(0.1459), PlayerUtil::fract16(0.1443), PlayerUtil::fract16(0.1429),
        PlayerUtil::fract16(0.1414), PlayerUtil::fract16(0.1400), PlayerUtil::fract16(0.1387), PlayerUtil::fract16(0.1374), PlayerUtil::fract16(0.1361),
        PlayerUtil::fract16(0.1348), PlayerUtil::fract16(0.1336), PlayerUtil::fract16(0.1325), PlayerUtil::fract16(0.1313), PlayerUtil::fract16(0.1302),
        PlayerUtil::fract16(0.1291), PlayerUtil::fract16(0.1280), PlayerUtil::fract16(0.1270), PlayerUtil::fract16(0.1260), PlayerUtil::fract16(0.1250)
    };
    static_assert(MAX_TGENS + 1 == sizeof(VOICE_ATTENUATION) / sizeof(VOICE_ATTENUATION[0]),
                  "VOICE_ATTENUATION must be indexable by the full voice count");

    // Percussion never responds to pitch bend, so its phase layout only needs to cover the
    // sample index. One waveform sample is PERC_FRACTION_SCALE phase units: the sample rate
    // conversion is (sourceRate * PERC_FRACTION_SCALE) / AUDIO_SAMPLE_RATE, and the reader in
    // synthTask() must use the very same shift. Melodic voices use INST_INDEX_SHIFT instead,
    // where one full 256 point cycle is 2^31 phase units.
    static constexpr uint32_t PERC_INDEX_SHIFT = 16;
    static constexpr uint32_t PERC_FRACTION_SCALE = 1u << PERC_INDEX_SHIFT;
    static constexpr uint32_t INST_INDEX_SHIFT = 23;
    static constexpr uint32_t INST_INDEX_MASK = 0xff;
    static constexpr uint32_t INST_PHASE_MASK = 0x7fffffff;
    static constexpr uint32_t INST_FRACTION_SHIFT = 31 - 8 - 16;

    // Equal-power pan: index 0..PAN_STEPS maps to (cos(a), sin(a)) with
    // a = idx / PAN_STEPS * pi/2, so L^2 + R^2 = 1 and a note keeps its
    // apparent loudness wherever it sits on the stage. Values are generated in
    // init() (a constexpr cos is not portable); index PAN_STEPS/2 is centre.
    static constexpr uint8_t PAN_STEPS = 32;

    // The drum bank has one shots up to 2.5 s long, so a kit pattern needs room for
    // a crash, an open hat and a kick tail to overlap while the hats keep firing.
    static constexpr uint8_t MAX_PERC_VOICES = 12;
    static constexpr uint8_t MAX_MELODIC_VOICES = MAX_TGENS - MAX_PERC_VOICES;
    // Envelope decay is exponential rather than a linear ramp: a struck string loses a constant
    // *proportion* of its amplitude per cycle, and a straight line to zero sounds like a fading
    // fader, not like an instrument. The proportion is held as a 2^20 fixed point, so a voice
    // steps by (multiplier * rate + carry) >> 20 each sample. The carry is what makes long
    // decays work: with a 16 bit envelope, a 3 s decay is a step of well under one count once the
    // note is quietly ringing, and without the accumulated remainder the voice would freeze at
    // around -18 dB instead of reaching silence.
    static constexpr int32_t DECAY_SHIFT = 20;
    static constexpr int32_t DECAY_CARRY_MASK = (1 << DECAY_SHIFT) - 1;
    // 2^20 * ln(1000): a rate of numerator / samples takes the envelope to -60 dB in that many
    // samples. The linear form is within 0.5% of the exact value for every decay we use.
    static constexpr int32_t DECAY_T60_NUMERATOR = 7239000;
    static constexpr int32_t DECAY_MAX_RATE = 16384;   // keeps multiplier * rate inside int32
    static constexpr int32_t DECAY_MIN_RATE = 37;      // and caps a ring at about 8 seconds
    // A string rings roughly as long as its period is long, so the table T60 is referred to this
    // pitch and scaled by the note's frequency: 440 Hz.
    static constexpr uint32_t DECAY_REFERENCE_FREQ4096 = 1802240;
    // -66 dBFS. A voice that has fallen this far is inaudible under anything else in the mix, so
    // it is retired instead of left holding a generator and ducking the bus for a key that is
    // still physically down.
    static constexpr int32_t SILENCE_LEVEL = 32;
    static_assert((int64_t)0x10000 * DECAY_MAX_RATE + DECAY_CARRY_MASK <= 0x7fffffff,
                  "the decay step must stay inside int32 at full envelope");
    static_assert(DECAY_MIN_RATE > 0 && DECAY_MIN_RATE < DECAY_MAX_RATE, "decay rate bounds");

    // The bus gain follows the voice count, but it is walked to its target instead of jumping:
    // 1 voice -> 2 voices is a 3 dB target change, and applying it in one sample would step every
    // sustaining voice on the bus. At 64 units per frame the whole change takes about 2 ms.
    static constexpr int32_t GAIN_SLEW_STEP = 64;
    // Master bus knee/compression: the summed bus is compressed 1:4 above the knee and hard
    // clamped below full scale, so coincident drum transients saturate instead of wrapping.
    // The knee sits just above the peak of one drum (PEAK_CEILING in convert_drums.py), so
    // a lone drum is reproduced exactly and only coincident voices are compressed.
    static constexpr int32_t SOFTCLIP_KNEE = 28500;
    static constexpr int32_t SOFTCLIP_CEILING = 32000;
    static_assert(MAX_MELODIC_VOICES + MAX_PERC_VOICES == MAX_TGENS, "bus budgets must fill the pool");

    // Identity mapping: MIDI program number directly indexes waveform array
    static constexpr uint8_t INSTRUMENT_PATCH_MAP[128] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
    };

    enum DrumIndex { // drum indexes into the drum bank
        D_KICK_TIGHT, D_KICK_DEEP, D_KICK_PUNCHY, D_KICK_SOFT,
        D_SNARE_TIGHT, D_SNARE_FAT, D_SNARE_RIMSHOT, D_SNARE_BRUSH,
        D_HAT_CLOSED_TIGHT, D_HAT_CLOSED_SOFT, D_HAT_OPEN_SHORT, D_HAT_OPEN_LONG,
        D_TOM_LOW, D_TOM_HIGH,
        D_CRASH, D_CLAP,
        D_COUNT
    };

    // The bank is loudness matched by tools/convert_drums.py, so this is a taste table rather
    // than a correction table: drop one entry to move that drum back in the kit.
    static constexpr int32_t DRUM_TRIM[D_COUNT] = {
        PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000),
        PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000),
        PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000),
        PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000), PlayerUtil::fract16(1.000)
    };

    // The drum kit is spread across the stage regardless of the MIDI pan on
    // channel 10: kick and snare stay anchored centre, the hats sit to the
    // right, the toms fan out and the crash leans left. Index into the
    // PAN_STEPS equal-power LUTs; a static_assert below guards the length.
    static constexpr uint8_t DRUM_PAN_INDEX[D_COUNT] = {
        /* kick */                  16, 16, 16, 16,
        /* snare */                 14, 14, 13, 17,
        /* hats */                  21, 20, 19, 18,
        /* tom low/high */          10, 24,
        /* crash, clap */           6, 14
    };

    // General MIDI percussion key map (pitch 35..81), by pitch, not by order of appearance.
    static constexpr DrumIndex DRUM_PATCH_MAP[128] = {
        /*  0*/ D_KICK_TIGHT, D_KICK_DEEP, D_KICK_PUNCHY, D_KICK_SOFT, D_SNARE_BRUSH, D_TOM_LOW, D_TOM_HIGH, D_KICK_TIGHT,
        /*  8*/ D_KICK_DEEP, D_KICK_PUNCHY, D_KICK_SOFT, D_SNARE_BRUSH, D_TOM_LOW, D_TOM_HIGH, D_KICK_TIGHT, D_KICK_DEEP,
        /* 16*/ D_KICK_PUNCHY, D_KICK_SOFT, D_SNARE_BRUSH, D_TOM_LOW, D_TOM_HIGH, D_KICK_TIGHT, D_KICK_DEEP, D_KICK_PUNCHY,
        /* 24*/ D_KICK_SOFT, D_SNARE_BRUSH, D_TOM_LOW, D_TOM_HIGH, D_KICK_TIGHT, D_KICK_DEEP, D_KICK_PUNCHY, D_KICK_SOFT,
        /* 32*/ D_SNARE_BRUSH, D_TOM_LOW, D_TOM_HIGH,
        /* 35*/ D_KICK_DEEP, D_KICK_TIGHT, D_SNARE_RIMSHOT, D_SNARE_TIGHT, D_CLAP, D_SNARE_FAT,
        /* 41*/ D_TOM_LOW, D_HAT_CLOSED_TIGHT, D_TOM_LOW, D_HAT_CLOSED_SOFT, D_TOM_LOW, D_HAT_OPEN_LONG,
        /* 47*/ D_TOM_HIGH, D_TOM_HIGH, D_CRASH, D_TOM_HIGH, D_CRASH, D_CRASH, D_CRASH, D_HAT_CLOSED_SOFT, D_CRASH,
        /* 56*/ D_SNARE_RIMSHOT, D_CRASH, D_CLAP, D_HAT_OPEN_SHORT, D_TOM_HIGH, D_TOM_HIGH, D_TOM_LOW, D_TOM_HIGH,
        /* 64*/ D_TOM_LOW, D_TOM_LOW, D_TOM_HIGH, D_TOM_LOW, D_SNARE_RIMSHOT, D_SNARE_RIMSHOT, D_HAT_CLOSED_SOFT, D_HAT_CLOSED_TIGHT,
        /* 72*/ D_TOM_HIGH, D_TOM_LOW, D_SNARE_RIMSHOT, D_SNARE_BRUSH, D_SNARE_RIMSHOT, D_SNARE_RIMSHOT, D_SNARE_RIMSHOT, D_SNARE_BRUSH,
        /* 80*/ D_SNARE_BRUSH, D_HAT_CLOSED_TIGHT, D_HAT_OPEN_SHORT, D_HAT_CLOSED_SOFT, D_HAT_CLOSED_TIGHT, D_SNARE_BRUSH, D_SNARE_BRUSH, D_HAT_CLOSED_TIGHT,
        /* 88*/ D_SNARE_TIGHT, D_HAT_OPEN_SHORT, D_TOM_LOW, D_CRASH, D_KICK_PUNCHY, D_HAT_CLOSED_TIGHT, D_SNARE_FAT, D_CLAP,
        /* 96*/ D_SNARE_TIGHT, D_HAT_OPEN_SHORT, D_TOM_LOW, D_CRASH, D_KICK_PUNCHY, D_HAT_CLOSED_TIGHT, D_SNARE_FAT, D_CLAP,
        /*104*/ D_SNARE_TIGHT, D_HAT_OPEN_SHORT, D_TOM_LOW, D_CRASH, D_KICK_PUNCHY, D_HAT_CLOSED_TIGHT, D_SNARE_FAT, D_CLAP,
        /*112*/ D_SNARE_TIGHT, D_HAT_OPEN_SHORT, D_TOM_LOW, D_CRASH, D_KICK_PUNCHY, D_HAT_CLOSED_TIGHT, D_SNARE_FAT, D_CLAP,
        /*120*/ D_SNARE_TIGHT, D_HAT_OPEN_SHORT, D_TOM_LOW, D_CRASH, D_KICK_PUNCHY, D_HAT_CLOSED_TIGHT, D_SNARE_FAT, D_CLAP
    };
    // The previous table had no pitch designators at all, so the entries meant for 35 and up
    // silently landed at index 0 and the whole kit played the wrong samples. These fail the
    // build if the table ever drifts out of GM alignment again.
    static_assert(DRUM_PATCH_MAP[35] == D_KICK_DEEP && DRUM_PATCH_MAP[36] == D_KICK_TIGHT,
                  "GM: 35/36 must be the bass drums");
    static_assert(DRUM_PATCH_MAP[37] == D_SNARE_RIMSHOT && DRUM_PATCH_MAP[38] == D_SNARE_TIGHT &&
                  DRUM_PATCH_MAP[39] == D_CLAP && DRUM_PATCH_MAP[40] == D_SNARE_FAT,
                  "GM: 37..40 must be side stick, snare, clap, snare");
    static_assert(DRUM_PATCH_MAP[42] == D_HAT_CLOSED_TIGHT && DRUM_PATCH_MAP[44] == D_HAT_CLOSED_SOFT &&
                  DRUM_PATCH_MAP[46] == D_HAT_OPEN_LONG,
                  "GM: 42/44/46 must be closed, pedal and open hi-hat");
    static_assert(DRUM_PATCH_MAP[45] == D_TOM_LOW && DRUM_PATCH_MAP[47] == D_TOM_HIGH &&
                  DRUM_PATCH_MAP[48] == D_TOM_HIGH && DRUM_PATCH_MAP[50] == D_TOM_HIGH,
                  "GM: 45/47/48/50 must be toms");
    static_assert(DRUM_PATCH_MAP[49] == D_CRASH && DRUM_PATCH_MAP[51] == D_CRASH &&
                  DRUM_PATCH_MAP[55] == D_CRASH && DRUM_PATCH_MAP[57] == D_CRASH,
                  "GM: 49/51/55/57 must be cymbals");
    static_assert(MAX_TGENS >= MAX_PERC_VOICES, "percussion voice cap must fit in the generator pool");
    static_assert(withinUnity(VOICE_ATTENUATION, MAX_TGENS + 1), "VOICE_ATTENUATION must stay at or below unity");
    static_assert(withinUnity(DRUM_TRIM, D_COUNT), "DRUM_TRIM must stay at or below unity");
    static_assert(sizeof(DRUM_PAN_INDEX) / sizeof(DRUM_PAN_INDEX[0]) == D_COUNT,
                  "one pan position per drum bank entry");
    static_assert(MIDI_CHANNELS > 0, "channel state needs at least one entry");

    int32_t BENDING_FACTOR_N[128];
    int32_t BENDING_FACTOR_P[128];
    int32_t DRUM_VELOCITY_LUT[128];
    
    ToneGenerator m_generators[MAX_TGENS];
    typedef struct {
        uint8_t channel;
        uint8_t pitch;
    } GeneratorBinding;
    GeneratorBinding m_generatorBindings[MAX_TGENS];
    
    uint8_t m_channelPatches[MIDI_CHANNELS];
    uint8_t m_channelVolumes[MIDI_CHANNELS];
    int32_t m_channelVolumesFract[MIDI_CHANNELS];
    int32_t m_channelPitchBendings[MIDI_CHANNELS];
    int32_t m_channelPanIndex[MIDI_CHANNELS];
    int32_t m_panLutL[PAN_STEPS + 1];
    int32_t m_panLutR[PAN_STEPS + 1];

    AudioTarget &m_audioTarget;
    FixedReverb m_reverb;
    uint16_t m_noteSerial;
    int32_t m_melodicGain;
    int32_t m_percussionGain;
    bool m_enablePercussion;
    bool m_onlyPercussion;
    bool m_reverbEnabled = true;
    inline int16_t allocateGenerator(bool percussion, uint8_t pitch);
    void fadeOutVoice(uint8_t channel, uint8_t pitch, uint32_t fadeSamples);
    void releaseBinding(uint8_t index);
    static inline int16_t softClip(int32_t sample);
    static inline int32_t slewGain(int32_t current, int32_t target);
    static inline int32_t decayRate(int32_t t60Samples, uint32_t freq4096,
                                    int32_t minRate = DECAY_MIN_RATE);
    void retireVoice(uint8_t generatorIndex);
    uint32_t m_onNotes;
    uint32_t m_offNotes;
    uint32_t m_outOfRangeNotes;
    uint32_t m_missedNotes;
    uint32_t m_stolenNotes;
    uint32_t m_retriggeredNotes;
    uint32_t m_idleTakes;
    uint32_t m_statsTimestamp;
    uint32_t m_statsBlocks;
    uint16_t m_blockFrames;
    uint32_t m_maxSynthDuration;
    uint32_t m_synthCount;
public:
    void setReverbEnabled(bool on) { m_reverbEnabled = on; }
    static constexpr uint32_t AUDIO_SAMPLE_RATE = 24000;
    // Frames rendered per synthTask() call, and the frame count asked of the audio pool. A MIDI
    // note can only start on a block boundary, so this is the grooving quantisation: 256 frames
    // at this rate smeared attacks over 10 ms, which is audible on a hi hat. Pool buffers keep the
    // same total slack (32 x 64 frames is the 85 ms that 8 x 256 was). synthTask() renders the
    // size the pool actually hands back, so the two only disagree if a caller passes something
    // else to the target; showStats() prints the block budget against the worst measured block.
    static constexpr uint16_t AUDIO_BLOCK_SAMPLES = 64;
    static constexpr uint16_t AUDIO_POOL_BUFFERS = 32;
    static constexpr uint8_t PERCUSSION_CHANNEL = 9;
    static constexpr uint8_t GM_CLOSED_HI_HAT = 42;
    static constexpr uint8_t GM_PEDAL_HI_HAT = 44;
    static constexpr uint8_t GM_OPEN_HI_HAT = 46;
    WaveTablePlayer(AudioTarget &m_audioTarget,
        bool enablePercussion = true,
        bool onlyPercussion = false);
    virtual bool init();
    virtual void playNote(uint8_t channel, uint8_t pitch, uint8_t velocity);
    virtual void stopNote(uint8_t channel, uint8_t pitch);
    virtual void allNotesOff();
    virtual void changeProgram(uint8_t channel, uint8_t program);
    virtual void setVolume(uint8_t channel, uint8_t volume);
    virtual void setPan(uint8_t channel, uint8_t pan);
    virtual void setPitchBending(uint8_t channel, uint16_t bending);
    virtual void synthTask();
    virtual void showStats();
    virtual uint16_t audioBufferCount() const;
    virtual uint16_t audioBufferSamples() const;
    void setEnablePercussion(bool enablePercussion);
    void setOnlyPercussion(bool onlyPercussion);
};

#endif /* WAVETABLEPLAYER_H */