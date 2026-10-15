#ifndef NOTE_WATCHDOG_H
#define NOTE_WATCHDOG_H

#include <cinttypes>

// Bounds how long a note may stay held with no MIDI traffic behind it.
//
// A voice is only ever released by a note-off, and a note-off only ever comes from a parsed
// MIDI message; nothing else bounds a held key. So when a stream stops mid-note -- a song is
// stopped while a chord is down, or an FPGA core that was feeding the link is replaced by one
// that is not -- the voice sustains forever. This is the missing bound: the caller reports
// every MIDI byte it sees, and expired() says once per quiet period that no more are coming and
// the keys should be let go.
//
// Activity is any byte at all, realtime and partial messages included: that is the most
// conservative signal there is, so the watchdog can only stay its hand while a stream is
// talking, never cut one that is.
//
// Kept free of any pico time dependency so the timing is host testable without the device, the
// same way MidiStreamParser is: the caller supplies the millisecond timestamp.
class NoteWatchdog {
public:
    // Ten seconds is a resting chord rather than a performance decision: the cost is
    // asymmetric. A note held forever is the bug, so a long window costs almost nothing where a
    // short one risks cutting a legitimate rest. This is the one number to turn if a core is
    // ever seen to pause longer than this between messages.
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 10000;

    explicit NoteWatchdog(uint32_t timeoutMs = DEFAULT_TIMEOUT_MS):
        m_timeoutMs(timeoutMs) {}

    // A MIDI byte has arrived: the stream is alive, so the clock restarts and the watchdog
    // re-arms.
    void activity(uint32_t nowMs) {
        m_lastActivityMs = nowMs;
        m_armed = true;
    }

    // True when the stream has been quiet for at least the timeout. Fires once per quiet period
    // -- the caller reacts to the edge, not the level -- and arms again only when activity() is
    // seen. It is disarmed until the first byte, so a link that never spoke never fires and boot
    // needs no timestamp. The subtraction is unsigned so the millisecond clock may wrap.
    bool expired(uint32_t nowMs) {
        if (!m_armed) {
            return false;
        }
        if (nowMs - m_lastActivityMs < m_timeoutMs) {
            return false;
        }
        m_armed = false;
        return true;
    }

    // Stop expecting traffic (used when the service is parked and will not be rendering).
    void disarm() {
        m_armed = false;
    }

    uint32_t timeoutMs() const {
        return m_timeoutMs;
    }

private:
    uint32_t m_timeoutMs;
    uint32_t m_lastActivityMs = 0;
    bool m_armed = false;
};

#endif /* NOTE_WATCHDOG_H */
