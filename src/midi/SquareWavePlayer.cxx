
#include "SquareWavePlayer.h"
#include <math.h>
#include "pico/stdlib.h"

const uint32_t SquareWavePlayer::COUNT_INC = ((long)1u) << COUNT_HEADROOM_BITS;

SquareWavePlayer::SquareWavePlayer(AudioTarget &audioTarget):
    m_audioTarget(audioTarget) {
}

void SquareWavePlayer::oscInit(const uint32_t sampleFrequency) {
    const double count_inc = COUNT_INC;
    const double log_note_step_ratio = log(OCTAVE_FREQ_RATIO) / NOTES_PER_OCTAVE;
    for (size_t osc = 0; osc < NUM_OSC; osc++) {
        const double osc_freq = A4_FREQ * exp((osc - (double)A4_NOTE_NUMBER) * log_note_step_ratio);
        // half (0.5) inc, since square wave elongation toggles twice per period
        const uint32_t count_wrap = round(0.5 * count_inc * sampleFrequency / osc_freq);
        OscStatus *osc_status = &m_oscStatuses[osc];
        osc_status->count_wrap = count_wrap;
        osc_status->count = 0;
        osc_status->velocity = 0;
        osc_status->elongation = 0;
    }
}

void SquareWavePlayer::stateInit() {
    for (size_t channel = 0; channel < NUM_CHN; channel++) {
        ChannelStatus *channelStatus = &m_midiStatus.channelStatus[channel];
        for (size_t note = 0; note < NUM_OSC; note++) {
            NoteStatus *noteStatus = &channelStatus->noteStatus[note];
            noteStatus->velocity = 0;
        }
    }
}

bool SquareWavePlayer::init() {
    m_timestampActiveSensing = time_us_64();
    oscInit(m_audioTarget.sampleFrequency());
    stateInit();
    return true;
}

void SquareWavePlayer::addToOscStatus(const uint8_t pitch,
                                      const int8_t deltaVelocity) {
    OscStatus *oscStatus = &m_oscStatuses[pitch];
    oscStatus->velocity += deltaVelocity;
    const int16_t elongation = oscStatus->elongation;
    if (elongation > 0) {
        oscStatus->elongation += deltaVelocity;
    } else if (elongation < 0) {
        oscStatus->elongation -= deltaVelocity;
    } else {
        oscStatus->elongation = deltaVelocity;
    }
}

void SquareWavePlayer::playNote(uint8_t channel, uint8_t pitch, uint8_t velocity) {
    if (channel == 9) {
        // The square player carries no drum kit: GM percussion (channel 10)
        // is disabled so a song's drum track does not turn into beeps at the
        // GM pitch-to-key numbers.
        return;
    }
    ChannelStatus *channelStatus = &m_midiStatus.channelStatus[channel];
    NoteStatus *noteStatus = &channelStatus->noteStatus[pitch];
    const uint8_t prevVelocity = noteStatus->velocity;
    noteStatus->velocity = velocity;
    addToOscStatus(pitch, velocity - prevVelocity);
}

void SquareWavePlayer::stopNote(uint8_t channel, uint8_t pitch) {
    if (channel == 9) {
        return;
    }
    ChannelStatus *channelStatus = &m_midiStatus.channelStatus[channel];
    NoteStatus *noteStatus = &channelStatus->noteStatus[pitch];
    const uint8_t velocity = noteStatus->velocity;
    noteStatus->velocity = 0;
    addToOscStatus(pitch, -velocity);
}

void SquareWavePlayer::showStats() {
}

uint16_t SquareWavePlayer::audioBufferCount() const {
    return AUDIO_POOL_BUFFERS;
}

uint16_t SquareWavePlayer::audioBufferSamples() const {
    return AUDIO_BLOCK_SAMPLES;
}

void SquareWavePlayer::synthTask() {
    struct audio_buffer *audio_buffer = m_audioTarget.takeAudioBuffer(false);
    if (!audio_buffer) {
        return;
    }
    const uint32_t audio_buffer_sample_count = audio_buffer->max_sample_count;
    if (!audio_buffer_sample_count) {
        return;
    }
    audio_buffer->sample_count = audio_buffer_sample_count;
    int16_t *out = (int16_t *) audio_buffer->buffer->bytes;
    const uint16_t vol_mul = round(2.0 * (((long)1u) << VOL_BITS));
    const size_t num_osc = NUM_OSC;
    const uint32_t count_inc = COUNT_INC;
    OscStatus *osc_statuses = m_oscStatuses;
    const uint32_t total_sample_count =
        audio_buffer->max_sample_count * (m_audioTarget.isStereo() ? 2 : 1);
    for (uint32_t sample_index = 0; sample_index < total_sample_count;) {
        int64_t sample_value = 0;
        for (size_t osc = 0; osc < num_osc; osc++) {
            OscStatus *osc_status = &osc_statuses[osc];
            uint32_t elongation = osc_status->elongation;
            if (elongation) {
                const uint32_t count_wrap = osc_status->count_wrap;
                uint32_t count = osc_status->count;
                count += count_inc;
                if (count >= count_wrap) {
                    count -= count_wrap;
                    elongation = -elongation;
                    osc_status->elongation = elongation;
                }
                osc_status->count = count;
                sample_value += elongation;
            }
        }
        const int16_t scaled_sample_value = (int16_t)((sample_value * vol_mul) >> VOL_BITS);
        if (m_audioTarget.isStereo()) {
            out[sample_index++] = scaled_sample_value; // left channel
            out[sample_index++] = scaled_sample_value; // right channel
        } else {
            out[sample_index++] = scaled_sample_value; // mono channel
        }
    }
    m_audioTarget.giveAudioBuffer(audio_buffer);
}