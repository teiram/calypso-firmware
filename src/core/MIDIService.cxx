#include "MIDIService.h"
#include "calypso-debug.h"
#include <cmath>

using namespace calypso;

MIDIService::MIDIService(AudioTarget &audioTarget, MIDIStateMachine &midiStateMachine):
    m_audioTarget(audioTarget),
    m_midiStateMachine(midiStateMachine) {}

const char *MIDIService::name() {
    return NAME;
}

bool MIDIService::init() {
    if (m_audioTarget.init(AudioTarget::DEFAULT_BUFFER_COUNT,
        AudioTarget::DEFAULT_BUFFER_SAMPLE_COUNT)) {
        return m_midiStateMachine.init(m_audioTarget.sampleFrequency());
    } else {
        return false;
    }
}

bool MIDIService::needsAttention() {
    return true;
}

void MIDIService::attention() {
    struct audio_buffer *audioBuffer = m_audioTarget.takeAudioBuffer(false);
    if (!audioBuffer) {
        return;
    }
    const uint32_t audioBufferSampleCount = audioBuffer->max_sample_count;
    if (!audioBufferSampleCount) {
        return;
    }
    bool stereo = m_audioTarget.isStereo();
    audioBuffer->sample_count = audioBufferSampleCount;
    int16_t *out = (int16_t *) audioBuffer->buffer->bytes;
    const uint16_t volMul = round(2.0 * (((long)1u) << VOL_BITS));
    const size_t numOsc = MIDIStateMachine::NUM_OSC;
    const uint32_t countInc = MIDIStateMachine::COUNT_INC;
    MIDIStateMachine::OscStatus *oscStatuses = m_midiStateMachine.oscStatuses();
    const uint32_t totalSampleCount = audioBuffer->max_sample_count * (stereo ? 2 : 1);
    for (uint32_t sampleIndex = 0; sampleIndex < totalSampleCount;) {
        int64_t sampleValue = 0;
        for (size_t osc = 0; osc < numOsc; osc++) {
            MIDIStateMachine::OscStatus *oscStatus = &oscStatuses[osc];
            uint32_t elongation = oscStatus->elongation;
            if (elongation) {
                const uint32_t countWrap = oscStatus->countWrap;
                uint32_t count = oscStatus->count;
                count += countInc;
                if (count >= countWrap) {
                    count -= countWrap;
                    elongation = -elongation;
                    oscStatus->elongation = elongation;
                }
                oscStatus->count = count;
                sampleValue += elongation;
            }
        }
        const int16_t scaledSampleValue = (int16_t)((sampleValue * volMul) >> VOL_BITS);
        if (stereo) {
            out[sampleIndex++] = scaledSampleValue; // left channel
            out[sampleIndex++] = scaledSampleValue; // right channel
        } else {
            out[sampleIndex++] = scaledSampleValue; // mono channel
        }
    }
    m_audioTarget.giveAudioBuffer(audioBuffer);
}

void MIDIService::accept(uint8_t midiByte) {
    m_midiStateMachine.addMidiByte(midiByte);
}

void MIDIService::cleanup() {}

