#ifndef AUDIO_TARGET_H
#define AUDIO_TARGET_H

#include <inttypes.h>
#include "pico/audio.h"

class AudioTarget {
public:
    static constexpr uint16_t DEFAULT_BUFFER_COUNT = 8;
    static constexpr uint16_t DEFAULT_BUFFER_SAMPLE_COUNT = 256;
    AudioTarget(const uint32_t sampleFrequency, const bool stereo);
    virtual bool init(const uint16_t bufferCount, const uint16_t bufferSampleCount) = 0;
    uint32_t sampleFrequency() const;
    bool isStereo() const;
    struct audio_buffer *takeAudioBuffer(const bool block);
    void giveAudioBuffer(audio_buffer_t *audio_buffer);

    // Stop driving the output pins and hand them back to whoever needs them: the I2S pins are
    // the tape's too, so a core reload has to release them. This is a park, not a teardown --
    // pico_audio_i2s claims its PIO state machine, DMA channel and shared IRQ handler in
    // audio_i2s_setup(), which cannot be run twice in one boot, so resume() re-arms the
    // peripheral instead of rebuilding it. A target with nothing to park leaves these empty.
    virtual void park() {}
    virtual void resume() {}
protected:

    struct audio_format m_targetAudioFormat = {
        .sample_freq = 0,
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 0,
    };
    struct audio_buffer_format m_targetAudioBufferFormat = {
        .format = &m_targetAudioFormat,
        .sample_stride = 0,
    };
    struct audio_buffer_pool *m_audioBufferPool;
};

#endif /* AUDIO_TARGET_H */

