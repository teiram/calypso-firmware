#include "I2SAudioTarget.h"
#include "pico/stdlib.h"
#include "pico/audio.h"
#include "calypso-debug.h"

I2SAudioTarget::I2SAudioTarget(const uint32_t sampleFrequency,
                   const uint8_t i2sClockBaseGpio,
                   const uint8_t i2sDataGpio,
                   const uint8_t pioSM):
    AudioTarget(sampleFrequency, true) {
    m_targetAudioConfig.clock_pin_base = i2sClockBaseGpio;
    m_targetAudioConfig.data_pin = i2sDataGpio;
    m_targetAudioConfig.pio_sm = pioSM;
}

bool I2SAudioTarget::init(const uint16_t bufferCount, const uint16_t bufferSampleCount) {
    m_audioBufferPool = audio_new_producer_pool(&m_targetAudioBufferFormat,
                            bufferCount, bufferSampleCount);
    const struct audio_format *outputAudioFormat =
      audio_i2s_setup(&m_targetAudioFormat, &m_targetAudioConfig);
    if (!outputAudioFormat) {
        MIDI_DEBUG_LOG(L_ERROR, "Error on audio_i2s_setup\n");
        return false;
    }

    // The producer pool is already stereo, so this is a stereo-to-stereo copy.
    // Match the consumer pool to the producer geometry: a note can only start on
    // a delivered block, so a larger consumer buffer would quantise note onsets.
    // Queue depth, not buffer size, is what keeps the DMA fed, and 8 x N frames
    // is the same audio in flight as the 2 x 2N the historical ratio produced.
    const uint16_t consumerBufferCount = 8;
    const uint16_t consumerBufferSamples = bufferSampleCount;
    const __unused bool ok = audio_i2s_connect_extra(m_audioBufferPool, false,
        consumerBufferCount, consumerBufferSamples, NULL);
    if (!ok) {
        MIDI_DEBUG_LOG(L_ERROR, "Error connecting i2s to producer pool\n");
        return false;
    }

    audio_i2s_set_enabled(true);

    return true;
}
