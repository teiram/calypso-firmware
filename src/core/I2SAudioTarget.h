#ifndef I2S_AUDIO_TARGET_H
#define I2S_AUDIO_TARGET_H

#include "AudioTarget.h"
#include "pico/audio_i2s.h"

class I2SAudioTarget : public AudioTarget {
public:
    I2SAudioTarget(const uint32_t sampleFrequency,
                   const uint8_t i2sClockBaseGpio,
                   const uint8_t i2sDataGpio,
                   const uint8_t pioSM);
    bool init(const uint16_t buffer_count = DEFAULT_BUFFER_COUNT,
        const uint16_t buffer_sample_count = DEFAULT_BUFFER_SAMPLE_COUNT);
private:
    struct audio_i2s_config m_targetAudioConfig = {
        .data_pin = 255,
        .clock_pin_base = 255,
        .dma_channel = 0,
        .pio_sm = 0,
    };
};

#endif /* I2S_AUDIO_TARGET_H */
