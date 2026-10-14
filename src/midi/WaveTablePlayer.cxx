#include "WaveTablePlayer.h"
#include <cstdio>
#include "PlayerUtil.h"
#include "pico/rand.h"
#include "pico/time.h"

#include "waveforms_flr3_128.c"
#include "fx_samples.c"

// Delays are defined in pulse counts per millisecond
constexpr int32_t ms2cnt(int32_t ms) {
    return ms * WaveTablePlayer::AUDIO_SAMPLE_RATE / 1000;
}

// The ADHSR envelope for every GM program, hand-authored. The cycle a program
// plays comes from the FLR3 row palette (waveforms_flr3_128.c) or, for the
// sampled programs, from fx_samples.c -- this table only shapes the note.
struct instrument_envelope_t {
  const int32_t delay, attack, hold, decay, release;  // count of of samples for envelope each phase
  const int32_t sustain_level;  // envelope level for sustain, as a fraction * 2^16

} INSTRUMENT_ENVELOPES[128] = {
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(3000), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(3000), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(3000), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(900), ms2cnt(80), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(0), ms2cnt(0), ms2cnt(100), PlayerUtil::fract16(1.000000)},
    {0, ms2cnt(5), ms2cnt(0), ms2cnt(0), ms2cnt(100), PlayerUtil::fract16(1.000000)},
    {0, ms2cnt(5), ms2cnt(0), ms2cnt(0), ms2cnt(100), PlayerUtil::fract16(1.000000)},
    {0, ms2cnt(5), ms2cnt(0), ms2cnt(0), ms2cnt(100), PlayerUtil::fract16(1.000000)},
    {0, ms2cnt(5), ms2cnt(0), ms2cnt(0), ms2cnt(100), PlayerUtil::fract16(1.000000)},
    {0, ms2cnt(5), ms2cnt(0), ms2cnt(0), ms2cnt(100), PlayerUtil::fract16(1.000000)},
    {0, ms2cnt(5), ms2cnt(0), ms2cnt(0), ms2cnt(100), PlayerUtil::fract16(1.000000)},
    {0, ms2cnt(5), ms2cnt(0), ms2cnt(0), ms2cnt(100), PlayerUtil::fract16(1.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(2000), ms2cnt(150), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(2000), ms2cnt(150), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1500), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1500), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1500), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1500), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1500), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1500), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1200), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1200), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1200), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1200), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1200), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1200), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1200), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(2), ms2cnt(1200), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(1200), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(30), ms2cnt(10), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(30), ms2cnt(10), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(30), ms2cnt(10), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(30), ms2cnt(10), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(30), ms2cnt(10), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(30), ms2cnt(10), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(30), ms2cnt(10), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(30), ms2cnt(10), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(50), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(50), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(50), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(50), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(50), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(50), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(50), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(50), ms2cnt(100), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(50), ms2cnt(10), ms2cnt(50), ms2cnt(200), PlayerUtil::fract16(0.800000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(2000), ms2cnt(150), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(2000), ms2cnt(150), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(2000), ms2cnt(150), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(2000), ms2cnt(150), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(20), ms2cnt(5), ms2cnt(30), ms2cnt(80), PlayerUtil::fract16(0.700000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(2000), ms2cnt(100), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(10), ms2cnt(20), PlayerUtil::fract16(0.400000)},
    {0, ms2cnt(5), ms2cnt(2), ms2cnt(10), ms2cnt(20), PlayerUtil::fract16(0.400000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(1500), ms2cnt(120), PlayerUtil::fract16(0.000000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
    {0, ms2cnt(10), ms2cnt(5), ms2cnt(30), ms2cnt(50), PlayerUtil::fract16(0.500000)},
};

#include "drums_k2.c"

#define drum_waveforms drum_waveforms_k2
#define drum_waveform_size drum_waveform_size_k2
#define drum_waveform_frequencies drum_waveform_frequencies_k2


WaveTablePlayer::WaveTablePlayer(AudioTarget &audioTarget,
    bool enablePercussion, bool onlyPercussion):
    m_audioTarget(audioTarget),
    m_noteSerial(0),
    m_melodicGain(0x10000),
    m_percussionGain(0x10000),
    m_enablePercussion(enablePercussion),
    m_onlyPercussion(onlyPercussion),
    m_onNotes(0),
    m_offNotes(0),
    m_outOfRangeNotes(0),
    m_missedNotes(0),
    m_stolenNotes(0),
    m_retriggeredNotes(0),
    m_idleTakes(0),
    m_statsTimestamp(0),
    m_statsBlocks(0),
    m_blockFrames(AUDIO_BLOCK_SAMPLES),
    m_maxSynthDuration(0),
    m_synthCount(0) {

    init();
}

bool WaveTablePlayer::init() {
    m_melodicGain = 0x10000;
    m_percussionGain = 0x10000;
    // equal-power pan: (cos a, sin a), a = idx/PAN_STEPS * pi/2, so a note keeps
    // its power wherever it sits on the stage. Must run before the generator
    // defaults below read the centre pair.
    const double halfPi = 1.57079632679489661923;
    for (uint8_t i = 0; i <= PAN_STEPS; i++) {
        const double a = (double) i / PAN_STEPS * halfPi;
        m_panLutL[i] = PlayerUtil::fract16(std::cos(a));
        m_panLutR[i] = PlayerUtil::fract16(std::sin(a));
    }
    for (uint8_t i = 0; i < MAX_TGENS; i++) {
        m_generators[i].playing = false;
        m_generators[i].percussion = false;
        m_generators[i].sampleVoice = false;
        m_generators[i].sampleLoops = false;
        m_generators[i].instrumentIndex = 0;
        m_generators[i].startSerial = 0;
        m_generators[i].envelopeRate = 0;
        m_generators[i].envelopeCarry = 0;
        // default to the centre of the stage until a note sets a real pan
        m_generators[i].panL = m_panLutL[PAN_STEPS / 2];
        m_generators[i].panR = m_panLutR[PAN_STEPS / 2];

        m_generatorBindings[i].channel = NO_CHANNEL;
        m_generatorBindings[i].pitch = NO_PITCH;
    }
    for (uint8_t i = 0; i < MIDI_CHANNELS; i++) {
        m_channelPatches[i] = 0;
        m_channelVolumes[i] = 100;
        m_channelVolumesFract[i] = PlayerUtil::fract16(100.0 / 127.0);
        m_channelPitchBendings[i] = 0x10000;
        m_channelPanIndex[i] = PAN_STEPS / 2;
    }

    for (uint8_t i = 0; i < 128; i++) {
        double factor = exp2((double) (2.0 * i) / (12.0 * 128));
        BENDING_FACTOR_N[127 - i] = PlayerUtil::fract16(1.0 / factor);
        BENDING_FACTOR_P[i] = PlayerUtil::fract16(factor);
    }
    // drums feel much more compressed in velocity than pitched instruments: a doubling of the
    // stick energy is far from a doubling of the perceived level, and linear MIDI velocity left
    // the whole kit sounding flat and uniformly loud on top of the rest of the mix.
    for (uint8_t i = 0; i < 128; i++) {
        double curve = pow((double) i / 127.0, 1.4);
        int32_t lut = PlayerUtil::fract16(curve);
        DRUM_VELOCITY_LUT[i] = lut > 0x10000 ? 0x10000 : (lut < 1 ? 1 : lut);
    }

    return true;
}

uint16_t WaveTablePlayer::audioBufferCount() const {
    return AUDIO_POOL_BUFFERS;
}

uint16_t WaveTablePlayer::audioBufferSamples() const {
    return AUDIO_BLOCK_SAMPLES;
}

void WaveTablePlayer::showStats() {
    uint8_t playingGenerators = 0;
    uint8_t idleGenerators = 0;
    uint8_t playingPercussion = 0;
    for (uint8_t i = 0; i < MAX_TGENS; i++) {
        if (m_generators[i].playing) {
            playingGenerators++;
            if (m_generators[i].percussion) {
                playingPercussion++;
            }
        } else {
            idleGenerators++;
        }
    }
    printf("voices %d/%d (drums %d) idle %d | on %lu off %lu oor %lu missed %lu stolen %lu\n",
        playingGenerators, MAX_TGENS, playingPercussion, idleGenerators,
        (unsigned long) m_onNotes, (unsigned long) m_offNotes, (unsigned long) m_outOfRangeNotes,
        (unsigned long) m_missedNotes, (unsigned long) m_stolenNotes);
    printf("  retriggered %lu (same key re-hit, expected on a fast pattern)\n",
        (unsigned long) m_retriggeredNotes);
    // The only honest dropout measure is blocks produced against the wall clock: the audio must
    // arrive at sampleRate / blockFrames per second whatever the main loop gets up to in between.
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    const uint32_t elapsed = now - m_statsTimestamp;
    const uint32_t produced = m_synthCount - m_statsBlocks;
    m_statsTimestamp = now;
    m_statsBlocks = m_synthCount;
    const uint64_t expected = (uint64_t) elapsed * AUDIO_SAMPLE_RATE / 1000 / m_blockFrames;
    const int64_t short_blocks = (int64_t) expected - (int64_t) produced;
    printf("  synth %lu blocks of %u frames: %ld short of real time (%u%%), %lu idle takes\n",
        (unsigned long) produced, m_blockFrames, (long) (short_blocks > 0 ? short_blocks : 0),
        (unsigned) (expected ? (uint64_t) produced * 100 / expected : 100), (unsigned long) m_idleTakes);
    printf("  worst block %lu us of %lu us budget\n",
        (unsigned long) ((m_maxSynthDuration + 500) / 1000),
        (unsigned long) (1000000ULL * m_blockFrames / AUDIO_SAMPLE_RATE));
    m_maxSynthDuration = 0; // per interval, so a single early spike does not hide forever
}

static inline bool issuedBefore(uint16_t a, uint16_t b) {
    return (int16_t) (a - b) < 0;
}

int16_t inline WaveTablePlayer::allocateGenerator(bool percussion, uint8_t pitch) {
    int16_t retrigger = -1;
    int16_t oldestOfBus = -1;
    int16_t oldestAny = -1;
    int16_t releasingOfBus = -1;
    uint8_t busVoices = 0;
    for (uint8_t i = 0; i < MAX_TGENS; i++) {
        ToneGenerator &tg = m_generators[i];
        if (!tg.playing) {
            continue;
        }
        if (tg.percussion == percussion) {
            busVoices++;
            if (percussion && m_generatorBindings[i].pitch == pitch &&
                (retrigger < 0 || issuedBefore(tg.startSerial, m_generators[retrigger].startSerial))) {
                retrigger = i;
            }
            if (tg.envelopeState == ENV_RELEASE && releasingOfBus < 0) {
                releasingOfBus = i; // already dying: the least harmful voice of this bus to take over
            }
            if (oldestOfBus < 0 || issuedBefore(tg.startSerial, m_generators[oldestOfBus].startSerial)) {
                oldestOfBus = i;
            }
        }
        if (oldestAny < 0 || issuedBefore(tg.startSerial, m_generators[oldestAny].startSerial)) {
            oldestAny = i;
        }
    }
    // A retriggered drum replaces its own previous hit: layered copies of the same sample were
    // comb filtering the hi-hats and cymbals into mud. This also covers a hit that arrives while
    // the previous one is still fading out.
    if (retrigger >= 0) {
        m_retriggeredNotes++; // re-hitting a key is not a voice being cut short
        return retrigger;
    }
    // Each bus keeps its own voice budget; the drums are never starved by a dense orchestral
    // score, and a dense score is never starved by ringing cymbals. The victim is therefore
    // always taken from the requesting bus, otherwise the budget would be broken by the steal.
    const uint8_t busCap = percussion ? MAX_PERC_VOICES : MAX_MELODIC_VOICES;
    if (busVoices >= busCap) {
        int16_t victim = releasingOfBus >= 0 ? releasingOfBus : oldestOfBus;
        if (victim >= 0) {
            m_stolenNotes++;
        }
        return victim;
    }
    for (uint8_t i = 0; i < MAX_TGENS; i++) {
        if (!m_generators[i].playing) {
            return i;
        }
    }
    // the pool is full of sustaining voices: give the oldest one up, whatever bus it is on
    int16_t victim = oldestOfBus >= 0 ? oldestOfBus : oldestAny;
    if (victim >= 0) {
        m_stolenNotes++;
    }
    return victim;
}

// Ramp a sounding voice down to silence and let the sample loop retire it. Used by the
// percussion note-off and by the hi-hat pedal, which must mute the open hat instead of
// leaving it ringing underneath the closed one. The binding stays in place while the voice
// fades, so that a following hit of the same key steals this voice instead of layering on it.
void WaveTablePlayer::fadeOutVoice(uint8_t channel, uint8_t pitch, uint32_t fadeSamples) {
    if (fadeSamples == 0) {
        fadeSamples = 1;
    }
    for (uint8_t i = 0; i < MAX_TGENS; i++) {
        ToneGenerator &tg = m_generators[i];
        if (tg.playing && tg.envelopeState != ENV_RELEASE &&
            m_generatorBindings[i].channel == channel && m_generatorBindings[i].pitch == pitch) {
            tg.envelopeState = ENV_RELEASE;
            tg.envelopeIncrement = -tg.envelopeMultiplier / (int32_t) fadeSamples;
        }
    }
}

int32_t inline WaveTablePlayer::slewGain(int32_t current, int32_t target) {
    // Stepping down first and clamping afterwards leaves the gain oscillating by one step every
    // sample whenever it is already where it belongs, which is amplitude modulation at half the
    // sample rate: exactly the hiss this is supposed to avoid.
    if (current < target) {
        current += GAIN_SLEW_STEP;
        return current < target ? target : current;
    }
    if (current > target) {
        current -= GAIN_SLEW_STEP;
        return current > target ? target : current;
    }
    return current;
}

int32_t inline WaveTablePlayer::decayRate(int32_t t60Samples, uint32_t freq4096,
                                          int32_t minRate) {
    if (t60Samples <= 0) {
        return DECAY_MAX_RATE; // nothing to decay through: end the stage at once
    }
    int64_t rate = (int64_t) DECAY_T60_NUMERATOR / t60Samples;
    // A string rings about as long as its period is long, so the table T60 is referred to A4 and
    // scaled by the note frequency: the lowest A of a piano holds for many seconds, the top
    // octave is gone in half one.
    rate = rate * freq4096 / DECAY_REFERENCE_FREQ4096;
    if (rate < minRate) {
        rate = minRate;
    }
    if (rate > DECAY_MAX_RATE) {
        rate = DECAY_MAX_RATE;
    }
    return (int32_t) rate;
}

// A voice has finished: silence it and give the generator, and its key, back. Called when a
// struck instrument has decayed out while its key is still held, which is exactly what a piano
// does and what the previous linear-then-hold envelope could not express.
void WaveTablePlayer::retireVoice(uint8_t generatorIndex) {
    ToneGenerator &tg = m_generators[generatorIndex];
    tg.envelopeMultiplier = 0;
    tg.envelopeIncrement = 0;
    tg.envelopeState = ENV_IDLE;
    tg.playing = false;
    m_generatorBindings[generatorIndex].channel = NO_CHANNEL;
    m_generatorBindings[generatorIndex].pitch = NO_PITCH;
}

int16_t inline WaveTablePlayer::softClip(int32_t sample) {
    if (sample > SOFTCLIP_KNEE) {
        sample = SOFTCLIP_KNEE + ((sample - SOFTCLIP_KNEE) >> 2);
        return sample > SOFTCLIP_CEILING ? SOFTCLIP_CEILING : sample;
    }
    if (sample < -SOFTCLIP_KNEE) {
        sample = -SOFTCLIP_KNEE + ((sample + SOFTCLIP_KNEE) >> 2);
        return sample < -SOFTCLIP_CEILING ? -SOFTCLIP_CEILING : sample;
    }
    return sample;
}

void WaveTablePlayer::setOnlyPercussion(bool onlyPercussion) {
    m_onlyPercussion = onlyPercussion;
}

void WaveTablePlayer::setEnablePercussion(bool enablePercussion) {
    m_enablePercussion = enablePercussion;
}

void WaveTablePlayer::playNote(uint8_t channel, uint8_t pitch, uint8_t velocity) {
    if (pitch < MIN_NOTE || pitch > MAX_NOTE) {
        m_outOfRangeNotes++;
        return;
    }
    bool percussion = channel == PERCUSSION_CHANNEL;
    if ((percussion && m_enablePercussion) || (!percussion && !m_onlyPercussion)) {
        int8_t index = allocateGenerator(percussion, pitch);
        if (index > -1) {
            ToneGenerator &tg = m_generators[index];
            m_onNotes++;
            if (percussion) {
                DrumIndex drumIndex = DRUM_PATCH_MAP[pitch];
                tg.waveform = drum_waveforms[drumIndex];
                //compute the increment to move from one sample point on the waveform to the next
                tg.toneIncrement = ((uint64_t) drum_waveform_frequencies[drumIndex] * PERC_FRACTION_SCALE) / (uint64_t) AUDIO_SAMPLE_RATE;
                tg.tonePhase = 0; // start at the beginning
                tg.sampleEndingIndex = drum_waveform_size_k2[drumIndex] - 1; // remember the end of the waveform
                tg.percussion = true;
                tg.sampleVoice = false;
                tg.envelopeMultiplier = DRUM_TRIM[drumIndex];
                tg.envelopeIncrement = 0;
                tg.envelopeRate = 0; // one shots fade linearly, on the shared ramp
                tg.envelopeCarry = 0;
                tg.envelopeCount = INT_MAX; // one shots end on the sample, not on the envelope
                tg.envelopeState = ENV_SUSTAIN;
                tg.midiChannel = PERCUSSION_CHANNEL;
                tg.instrumentIndex = 0;
                tg.volume = DRUM_VELOCITY_LUT[velocity & 0x7f];
                // the kit is spread in hardware: panning is per drum, not per
                // channel, and CC10 on channel 10 is ignored
                tg.panL = m_panLutL[DRUM_PAN_INDEX[drumIndex]];
                tg.panR = m_panLutR[DRUM_PAN_INDEX[drumIndex]];
                if (pitch == GM_CLOSED_HI_HAT || pitch == GM_PEDAL_HI_HAT) {
                    // the pedal has to choke a ringing open hat, otherwise both sound at once
                    fadeOutVoice(PERCUSSION_CHANNEL, GM_OPEN_HI_HAT, ms2cnt(5));
                }
            } else {
                InstrumentIndex instrumentIndex = INSTRUMENT_PATCH_MAP[m_channelPatches[channel]];
                tg.instrumentIndex = instrumentIndex;
                tg.midiChannel = channel & 0x0f;
                tg.percussion = false;
                tg.volume = ((int32_t)(velocity & 0x7f) + 1) << 9; // From 01 << 9 to 80 << 9 -> 
                                                                   // 0x200 to 0x10000
                // per channel pan, applied per voice; equal-power with the
                // default centre at index PAN_STEPS/2
                tg.panL = m_panLutL[m_channelPanIndex[channel & 0x0f]];
                tg.panR = m_panLutR[m_channelPanIndex[channel & 0x0f]];

                const voice_sample_t &voice = INSTRUMENT_VOICES[instrumentIndex];
                if (voice.samples) {
                    // A recorded sample voice, for a program whose content the
                    // cycle palette cannot represent. It carries its own
                    // amplitude, so it holds the shared ramp flat at unity: a
                    // one-shot rings out to its end, a loop is faded by the
                    // note-off. Sample and loop ends are inclusive frames.
                    tg.waveform = voice.samples;
                    tg.sampleEndingIndex = voice.loopEnd;
                    tg.sampleLoopStart = voice.loopStart;
                    tg.sampleLoops = voice.loops;
                    tg.sampleVoice = true;
                    // root 0 plays at the recording's own rate: the increment is
                    // exactly one sample per frame, which is the reader's unity
                    // fast path (no interpolation, so no added aliasing). A
                    // rooted sample is transposed by the ratio to its root.
                    tg.toneIncrement = voice.root
                        ? ((uint64_t) FREQ4096[pitch - MIN_NOTE] * PERC_FRACTION_SCALE)
                              / (uint64_t) FREQ4096[voice.root - MIN_NOTE]
                        : (uint32_t) PERC_FRACTION_SCALE;
                    tg.tonePhase = 0; // start at the beginning
                    tg.envelopeMultiplier = 0x10000;
                    tg.envelopeIncrement = 0;
                    tg.envelopeRate = 0;        // a sample voice fades on the shared ramp
                    tg.envelopeCarry = 0;
                    tg.envelopeCount = INT_MAX; // it does not run the AHDSR
                    tg.envelopeState = ENV_SUSTAIN;
                } else {
                    // Per-key cycle rows from the soundfont's multisample layout.
                    // Every program that is not a sample voice has rows: the
                    // generator leaves the sampled programs empty on purpose
                    // (--sample-programs) and check_voices.py asserts the split,
                    // so a row always exists here.
                    tg.sampleVoice = false;
                    const waveform_rows_t &rows = FLR3_ROWS[instrumentIndex];
                    uint8_t row = 0;
                    while (row + 1 < rows.rowCount && pitch > rows.keyHigh[row]) {
                        row++;
                    }
                    tg.waveform = rows.cycles[row];
                    tg.tableShift = 8;
                    tg.envelopeAttack = INSTRUMENT_ENVELOPES[instrumentIndex].attack;
                    tg.envelopeHold = INSTRUMENT_ENVELOPES[instrumentIndex].hold;
                    tg.envelopeDecay = INSTRUMENT_ENVELOPES[instrumentIndex].decay;
                    tg.envelopeRelease = INSTRUMENT_ENVELOPES[instrumentIndex].release;
                    tg.envelopeSustainLevel = INSTRUMENT_ENVELOPES[instrumentIndex].sustain_level;
                    tg.envelopeMultiplier = 0; // setup AHDSR envelope
                    tg.envelopeCount = INSTRUMENT_ENVELOPES[instrumentIndex].delay; // # of samples
                    // could be zero, but that will get dealt with at the first sample time.
                    tg.envelopeState = ENV_DELAY;
                    tg.envelopeIncrement = 0;
                    // The decay the instrument was given is a time at A4; the note being played
                    // decides how much longer or shorter the string actually rings.
                    tg.envelopeRate = decayRate(INSTRUMENT_ENVELOPES[instrumentIndex].decay,
                                                FREQ4096[pitch - MIN_NOTE]);
                    tg.envelopeCarry = 0;

                    //compute the increment to move from one sample point on the waveform to the next
                    // f[note] * 0x1000 * 0x80000 -> (f[note] << 12) << 20
                    tg.toneIncrement = ((uint64_t) FREQ4096[pitch - MIN_NOTE] * 0x80000) / (uint64_t) AUDIO_SAMPLE_RATE;
                    //start at random place in the wave cycle to minimize phase lock cancellations.
                    //The phase must be inside the table *before* the first sample is read: the raw
                    //32 bit random value indexed past the 256 point waveform on every attack.
                    tg.tonePhase = get_rand_32() & INST_PHASE_MASK;
                }
            }
            tg.startSerial = m_noteSerial++;
            m_generatorBindings[index].channel = channel;
            m_generatorBindings[index].pitch = pitch;
            tg.playing = true; 
            //printf("Registered binding %02x/%02x\n", channel, pitch);
        } else {
            m_missedNotes++;
        }
    }
}

void WaveTablePlayer::stopNote(uint8_t channel, uint8_t pitch) {
    if (pitch < MIN_NOTE || pitch > MAX_NOTE) {
        return;
    }
    m_offNotes++;
    if (channel == PERCUSSION_CHANNEL) {
        // A one shot rings until its sample ends, so a note off only trims the tail. The
        // instrument AHDSR must not be applied here: a percussion voice carries no meaningful
        // instrumentIndex, and stepping its multiplier straight to the sustain level clicked.
        // The binding is kept so that a following hit of the same key steals this voice.
        fadeOutVoice(channel, pitch, ms2cnt(20));
        return;
    }
    // Every voice bound to that key, not just the first: two generators can end up on one key if
    // a note on arrives while an earlier one of the same pitch is still sounding, and releasing
    // only the first of them left the second sustaining with nothing left to stop it.
    for (uint8_t i = 0; i < MAX_TGENS; i++) {
        if (m_generatorBindings[i].channel == channel && m_generatorBindings[i].pitch == pitch) {
            ToneGenerator &tg = m_generators[i];
            if (tg.sampleVoice) {
                // A sample voice carries its own amplitude, not an instrument
                // AHDSR: a one-shot rings out to its end and a note-off only
                // clears the key binding; a loop is faded and the reader retires
                // it once the fade reaches silence.
                if (tg.sampleLoops) {
                    tg.envelopeState = ENV_RELEASE;
                    tg.envelopeIncrement = -tg.envelopeMultiplier / (int32_t) ms2cnt(20);
                }
                m_generatorBindings[i].channel = NO_CHANNEL;
                m_generatorBindings[i].pitch = NO_PITCH;
                continue;
            }
            // Release from wherever the envelope has actually got to. The old code restamped the
            // sustain level here, which is harmless while a held note sits at exactly that level
            // but would have made a decayed note jump louder the moment it was let go.
            tg.envelopeState = ENV_RELEASE;
            tg.envelopeCount = INT_MAX; // the release stage runs down to silence
            tg.envelopeRate = decayRate(tg.envelopeRelease,
                                        DECAY_REFERENCE_FREQ4096); // a damper does not scale
            tg.envelopeCarry = 0;
            m_generatorBindings[i].channel = NO_CHANNEL;
            m_generatorBindings[i].pitch = NO_PITCH;
        }
    }
}

void WaveTablePlayer::changeProgram(uint8_t channel, uint8_t program) {
    if (channel < MIDI_CHANNELS) {
        m_channelPatches[channel] = program;
    } 
}

void WaveTablePlayer::setVolume(uint8_t channel, uint8_t volume) {
    if (channel < MIDI_CHANNELS) {
        m_channelVolumes[channel] = volume & 0x7f;
        m_channelVolumesFract[channel] = PlayerUtil::fract16((double)(volume & 0x7f) / 127.0);
    }
}

void WaveTablePlayer::setPan(uint8_t channel, uint8_t pan) {
    if (channel < MIDI_CHANNELS) {
        // MIDI pan 0..127 maps onto the PAN_STEPS LUT; 64 is dead centre
        m_channelPanIndex[channel] = (int32_t) (pan & 0x7f) * PAN_STEPS / 127;
    }
}

void WaveTablePlayer::setPitchBending(uint8_t channel, uint16_t bending) {
    if (channel < MIDI_CHANNELS) {
        // MIDI pitch bend: 14-bit value, center at 0x2000 (8192)
        // Range: 0 = -2 semitones, 0x2000 = 0, 0x3FFF = +2 semitones
        // Calculate semitones: (bending - 8192) / 8192 * 2.0 = semitones
        int16_t bendSemitones = (int16_t)bending - 8192;
        
        // Convert to factor: factor = 2^(semitones/12)
        double factor = exp2((double)bendSemitones * 2.0 / (12.0 * 8192.0));
        m_channelPitchBendings[channel] = PlayerUtil::fract16(factor);
    }
}

void WaveTablePlayer::synthTask() {
    struct audio_buffer *ab = m_audioTarget.takeAudioBuffer(false);
    if (!ab) {
        m_idleTakes++; // the main loop outran the audio clock; the DMA has not asked for a
                       // block yet, which is normal, not a dropout
        return;
    }
    m_synthCount++;
    absolute_time_t startTime = get_absolute_time();
    int16_t *samples = (int16_t *) ab->buffer->bytes;
    const uint32_t frames = ab->max_sample_count;
    m_blockFrames = (uint16_t) frames;
    for (uint32_t sample = 0; sample < frames * 2; sample += 2) {
        // Both buses sum in 32 bits: the int16 accumulator this replaces wrapped
        // around every time the voice count changed, which clicked on practically
        // each note onset. The buses are stereo now: each voice is panned with
        // an equal-power L/R pair, so a voice contributes panL*voice to the left
        // sum and panR*voice to the right one. Centre (0.707 each) preserves
        // power; the attenuation that used to double the voice into both ears is
        // no longer there, so a lone centre voice reaches each channel at 0.707
        // of the old level.
        int32_t melodicSumL = 0, melodicSumR = 0;
        int32_t percussionSumL = 0, percussionSumR = 0;
        uint8_t melodicCount = 0;
        uint8_t percussionCount = 0;
        for (uint8_t tgen = 0; tgen < MAX_TGENS; tgen++) {
            ToneGenerator &tg = m_generators[tgen];
            if (tg.playing) {
                uint32_t index1, index2, scale;
                int16_t generatorLevel;
                if (tg.percussion) {
                    // percussion: play the waveform once
                    // tone_phase = iiiiiiiiiiiiiiiiiffffffffffffffff, i=index into waveform array, f=fraction
                    index1 = tg.tonePhase >> PERC_INDEX_SHIFT; // one waveform sample per PERC_FRACTION_SCALE
                    if (index1 > tg.sampleEndingIndex) {
                        index1 = tg.sampleEndingIndex; // never read past the end of the drum sample
                    }
                    index2 = index1 + 1;
                    //printf("index:%d, sampleEndingIndex:%d\n", index2, tg.sampleEndingIndex);
                    // sampleEndingIndex is the last sample that belongs to the one shot, so
                    // the voice is retired once it has been emitted: testing index2 here instead
                    // silently dropped the final sample of every drum.
                    if (index1 >= tg.sampleEndingIndex) {
                        tg.playing = false; // end of percussion waveform; stop playing soon
                        m_generatorBindings[tgen].channel = NO_CHANNEL;
                        m_generatorBindings[tgen].pitch = NO_PITCH;
                    }
                    if (index2 > tg.sampleEndingIndex) {
                        index2 = tg.sampleEndingIndex; // never read past the end of the drum sample
                    }
                    scale = tg.tonePhase & (PERC_FRACTION_SCALE - 1); // fractional distance between samples
                    if (tg.envelopeState == ENV_RELEASE && tg.envelopeMultiplier <= 0) {
                        // a choked hat or a released drum: the ramp down is stepped by the shared
                        // envelope path below; stop as soon as the voice is inaudible instead of
                        // running the rest of the one shot
                        tg.envelopeMultiplier = 0;
                        tg.playing = false;
                        m_generatorBindings[tgen].channel = NO_CHANNEL;
                        m_generatorBindings[tgen].pitch = NO_PITCH;
                    }
                } else if (tg.sampleVoice) {
                    // an instrument-driven sample voice: the same 16-bit index
                    // reader as the drums, but it runs on the melodic bus and a
                    // loop wraps back to sampleLoopStart instead of ending.
                    index1 = tg.tonePhase >> PERC_INDEX_SHIFT;
                    if (tg.sampleLoops) {
                        if (index1 > tg.sampleEndingIndex) {
                            // fold the overshoot back into the loop, keeping the
                            // fractional part so the pitch does not step
                            const uint32_t loopLength =
                                tg.sampleEndingIndex - tg.sampleLoopStart + 1;
                            index1 = tg.sampleLoopStart +
                                (index1 - tg.sampleLoopStart) % loopLength;
                            tg.tonePhase = (index1 << PERC_INDEX_SHIFT) |
                                (tg.tonePhase & (PERC_FRACTION_SCALE - 1));
                        }
                        index2 = index1 + 1;
                        if (index2 > tg.sampleEndingIndex) {
                            index2 = tg.sampleLoopStart; // the wrap is the loop's next sample
                        }
                    } else {
                        if (index1 >= tg.sampleEndingIndex) {
                            index1 = tg.sampleEndingIndex; // emit the final frame, then retire
                            tg.playing = false;
                            m_generatorBindings[tgen].channel = NO_CHANNEL;
                            m_generatorBindings[tgen].pitch = NO_PITCH;
                        }
                        index2 = index1 + 1;
                        if (index2 > tg.sampleEndingIndex) {
                            index2 = tg.sampleEndingIndex; // never read past the end
                        }
                    }
                    scale = tg.tonePhase & (PERC_FRACTION_SCALE - 1);
                    if (tg.envelopeState == ENV_RELEASE && tg.envelopeMultiplier <= 0) {
                        // a released loop: the note-off fade has reached silence
                        tg.envelopeMultiplier = 0;
                        tg.playing = false;
                        m_generatorBindings[tgen].channel = NO_CHANNEL;
                        m_generatorBindings[tgen].pitch = NO_PITCH;
                    }
                } else { 
                    // a wavetable cycle: repeat the instrument waveform indefinitely
                    // tone_phase = +iiiiiiiiffffffffffffffffxxxxxxx, i=index into waveform array, f=fraction
                    // tableShift keeps the per-voice cycle length: 256 point cycles
                    // shift by 31-8=23, so a longer table needs no new concept here.
                    const uint32_t tableMask = (1u << tg.tableShift) - 1u;
                    index1 = tg.tonePhase >> (31 - tg.tableShift); // index into the cycle
                    index2 = (index1 + 1) & tableMask;  // wrap around at the end
                    scale = (tg.tonePhase >> (31 - tg.tableShift - 16)) & (PERC_FRACTION_SCALE - 1);  // 16 bits of fractional distance between samples
                    while (tg.envelopeCount == 0) { // change to a state with a non-zero count
                        switch (tg.envelopeState) {
                        case ENV_IDLE:
                            tg.envelopeCount = INT_MAX;
                            break;
                        case ENV_DELAY:
                            tg.envelopeState = ENV_ATTACK;
                            tg.envelopeCount = tg.envelopeAttack;
                            tg.envelopeIncrement = 0x10000 / tg.envelopeCount; // ratchet up to maximum volume
                            break;
                        case ENV_ATTACK:
                            tg.envelopeState = ENV_HOLD;
                            tg.envelopeCount = tg.envelopeHold;
                            tg.envelopeMultiplier = 0x10000; // hold this volume
                            tg.envelopeIncrement = 0;
                            break;
                        case ENV_HOLD:
                            tg.envelopeState = ENV_DECAY;
                            tg.envelopeCount = INT_MAX; // the decay stage runs down to a level
                            tg.envelopeMultiplier = 0x10000; // start with max volume
                            tg.envelopeIncrement = 0;
                            tg.envelopeCarry = 0;
                            break;
                        case ENV_DECAY:
                            tg.envelopeState = ENV_SUSTAIN;
                            tg.envelopeCount = INT_MAX;
                            tg.envelopeMultiplier = tg.envelopeSustainLevel;
                            tg.envelopeIncrement = 0; // maintain the sustain volume level
                            break;
                        case ENV_SUSTAIN:
                            tg.envelopeCount = INT_MAX; // (shouldn't happen; just keep on keeping on)
                            break;
                        case ENV_RELEASE:
                            tg.envelopeState = ENV_IDLE;
                            tg.playing = false; // end of release: stop playing the note
                            break;
                        }
                    }
                    --tg.envelopeCount;

                    if (tg.envelopeState == ENV_DECAY || tg.envelopeState == ENV_RELEASE) {
                        const int32_t sustain = tg.envelopeSustainLevel;
                        if (tg.envelopeMultiplier <= SILENCE_LEVEL ||
                            (tg.envelopeState == ENV_DECAY && tg.envelopeMultiplier <= sustain)) {
                            if (tg.envelopeState == ENV_DECAY && sustain > SILENCE_LEVEL) {
                                tg.envelopeMultiplier = sustain; // a held instrument latches
                                tg.envelopeIncrement = 0;
                                tg.envelopeState = ENV_SUSTAIN;
                                tg.envelopeCount = INT_MAX;
                            } else {
                                retireVoice(tgen); // at rest: free the generator and its key
                                continue;
                            }
                        } else {
                            // Recompute the step from the level every sample, carrying the
                            // remainder over: see DECAY_SHIFT in the header.
                            int32_t delta = tg.envelopeMultiplier * tg.envelopeRate + tg.envelopeCarry;
                            tg.envelopeCarry = delta & DECAY_CARRY_MASK;
                            tg.envelopeIncrement = -(delta >> DECAY_SHIFT);
                        }
                    }
                }

                // The drum bank and a fixed-pitch (root 0) sample voice are at the
                // output rate, so the voice steps exactly one sample per frame and
                // the second tap and both multiplies are pure cost: the drums are
                // the loudest things in the mix and the least welcome place to run
                // out of CPU. Pitch bend can still take a sampled voice off unity,
                // in which case it interpolates like a cycle.
                uint32_t toneIncrement = tg.percussion ? tg.toneIncrement :
                    (uint32_t) PlayerUtil::fract32Mul(m_channelPitchBendings[tg.midiChannel], (int32_t) tg.toneIncrement);
                int16_t sample1 = tg.waveform[index1];
                if ((tg.percussion || tg.sampleVoice) && toneIncrement == PERC_FRACTION_SCALE) {
                    generatorLevel = sample1;
                } else {
                    const int32_t weight = (int32_t) (PERC_FRACTION_SCALE - scale);
                    int32_t val1 = sample1 * weight;
                    int32_t val2 = tg.waveform[index2] * (int32_t) scale;
                    generatorLevel = (int16_t) ((val1 + val2) >> PERC_INDEX_SHIFT);
                }
                // instruments and loops wrap; percussion and one-shot samples run forward
                if (tg.percussion) {
                    tg.tonePhase = tg.tonePhase + toneIncrement;
                } else if (tg.sampleVoice) {
                    tg.tonePhase = tg.tonePhase + toneIncrement; // the read step above bounds a loop
                } else {
                    tg.tonePhase = (tg.tonePhase + toneIncrement) & INST_PHASE_MASK;
                }
                generatorLevel = PlayerUtil::fract16Mul(tg.envelopeMultiplier, generatorLevel);  // envelope amplitude attenuation
                tg.envelopeMultiplier += tg.envelopeIncrement; // adjust attenuator
                if (tg.envelopeMultiplier > 0x10000) {
                    tg.envelopeMultiplier = 0x10000; // attack overshoot would widen the next product
                }
                // Scale this voice by velocity volume * channel volume * envelope; the mixer
                // attenuation belongs to the bus sum, so that a voice does not change level
                // when an unrelated voice starts or stops.
                int32_t voiced = PlayerUtil::fract16Mul(tg.volume,
                    PlayerUtil::fract16Mul(m_channelVolumesFract[tg.midiChannel], generatorLevel));
                if (tg.percussion) {
                    percussionSumL += (voiced * tg.panL) >> 16;
                    percussionSumR += (voiced * tg.panR) >> 16;
                    percussionCount++;
                } else {
                    melodicSumL += (voiced * tg.panL) >> 16;
                    melodicSumR += (voiced * tg.panR) >> 16;
                    melodicCount++;
                }
            }
        }
        m_melodicGain = slewGain(m_melodicGain, VOICE_ATTENUATION[melodicCount]);
        m_percussionGain = slewGain(m_percussionGain, VOICE_ATTENUATION[percussionCount]);
        int32_t melodicLevelL = PlayerUtil::fract32Mul(m_melodicGain, melodicSumL);
        int32_t melodicLevelR = PlayerUtil::fract32Mul(m_melodicGain, melodicSumR);
        int32_t percussionLevelL = PlayerUtil::fract32Mul(m_percussionGain, percussionSumL);
        int32_t percussionLevelR = PlayerUtil::fract32Mul(m_percussionGain, percussionSumR);
        const int32_t dryL = melodicLevelL + percussionLevelL;
        const int32_t dryR = melodicLevelR + percussionLevelR;
        // One recirculating plate, fed the mono sum, is sent wide by giving the
        // even combs to the left wet tap and the odd combs to the right one:
        // two channels of sound for a single FDN. The wet level rides the dry
        // mix, and both go through the same converging soft clipper so a loud
        // note and its tail cannot wrap the sample.
        int32_t wetL = 0, wetR = 0;
        if (m_reverbEnabled) {
            m_reverb.process((dryL + dryR) >> 1, wetL, wetR);
        }
        samples[sample] = softClip(dryL + wetL);
        samples[sample + 1] = softClip(dryR + wetR);
    }
    ab->sample_count = ab->max_sample_count;
    uint64_t elapsedTime = absolute_time_diff_us(startTime, get_absolute_time());
    m_maxSynthDuration = elapsedTime > m_maxSynthDuration ? elapsedTime : m_maxSynthDuration;
    m_audioTarget.giveAudioBuffer(ab);
}



