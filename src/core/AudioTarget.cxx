#include "AudioTarget.h"
#include "pico/stdlib.h"
#include "pico/audio.h"


AudioTarget::AudioTarget(const uint32_t sampleFrequency, const bool stereo) {
  m_targetAudioFormat.sample_freq = sampleFrequency;
  if (stereo) {
    m_targetAudioFormat.channel_count = 2;
    m_targetAudioBufferFormat.sample_stride = 4;
  } else {
    m_targetAudioFormat.channel_count = 1;
    m_targetAudioBufferFormat.sample_stride = 2;
  }
  m_audioBufferPool = nullptr;
}


uint32_t AudioTarget::sampleFrequency() const {
  return m_targetAudioFormat.sample_freq;
}

bool AudioTarget::isStereo() const {
  return m_targetAudioFormat.channel_count == 2;
}

struct audio_buffer* AudioTarget::takeAudioBuffer(const bool block) {
  return ::take_audio_buffer(m_audioBufferPool, block);
}

void AudioTarget::giveAudioBuffer(audio_buffer_t *audioBuffer) {
  ::give_audio_buffer(m_audioBufferPool, audioBuffer);
}
