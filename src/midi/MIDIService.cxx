#include "MIDIService.h"
#include "pico/time.h"
#include "calypso-extern.h"

using namespace calypso;

MIDIService::MIDIService(AudioTarget &audioTarget, Player &player):
    m_audioTarget(audioTarget),
    m_player(player),
    m_midiParser(player) {}

const char *MIDIService::name() {
    return NAME;
}

static uint32_t nowMs() {
    return to_ms_since_boot(get_absolute_time());
}

bool MIDIService::init() {
    if (m_bringUp != BringUp::NOT_TRIED) {
        return m_bringUp == BringUp::DONE;
    }
    // The pool geometry is the engine's: the wavetable mixer quantises a note
    // start to a block, so it asks for short blocks (there is no sampler here;
    // the point is the note-onset quantisation, not latency).
    //
    // Latch whatever the attempt produces: a failed bring-up is fatal for this boot rather than
    // something to retry on the next byte, because the audio target's setup cannot be repeated.
    const bool audioUp = m_audioTarget.init(m_player.audioBufferCount(),
                                            m_player.audioBufferSamples());
    const bool ok = audioUp && m_player.init();
    m_bringUp = ok ? BringUp::DONE : BringUp::FAILED;
    if (!ok) {
        return false;
    }
    audio_core1_enable(true);
    m_running = true;
    return true;
}

bool MIDIService::start() {
    const bool ok = init();
    // Register even on a failed bring-up, as the serial path always did: the
    // service object then stays in the list and accepts a later byte.
    if (!m_registered) {
        Service::registerService(this);
        m_registered = true;
    }
    return ok;
}

bool MIDIService::needsAttention() {
    // Silent while parked: there is nothing to render and the output pins are someone else's.
    return m_running;
}

void MIDIService::keepAlive() {
    if (m_running) {
        m_watchdog.activity(nowMs());
    }
}

void MIDIService::attention() {
    // Checked before synthTask(), which returns at once when the pool has no free buffer: the
    // watchdog must fire on time even when audio is not flowing for some other reason.
    if (m_watchdog.expired(nowMs())) {
        m_player.allNotesOff();
    }
    m_player.synthTask();
}

void MIDIService::accept(uint8_t midiByte) {
    if (m_bringUp != BringUp::DONE && !init()) {
        return;
    }
    if (!m_running) {
        // First byte after a park: take the output pins and the audio path back.
        m_audioTarget.resume();
        audio_core1_enable(true);
        m_running = true;
    }
    // Any byte counts as the stream being alive, realtime and partial messages included.
    m_watchdog.activity(nowMs());
    m_midiParser.accept(midiByte);
}

void MIDIService::stop() {
    if (!m_running) {
        return;
    }
    m_player.allNotesOff();
    audio_core1_enable(false);
    m_audioTarget.park();
    m_running = false;
    m_watchdog.disarm();
}

void MIDIService::cleanup() {
    stop();
}
