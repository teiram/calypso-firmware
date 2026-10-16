#ifndef MIDI_RECEIVER_H
#define MIDI_RECEIVER_H

#ifdef __cplusplus
extern "C" {
#endif
    #include <inttypes.h>
    
    void midi_byte_processor(uint8_t value);

    // Called when the FPGA is about to be reprogrammed. The core that was feeding the MIDI
    // link is gone, so no note-off is coming for whatever it left sounding, and the shared
    // output pins may be wanted by the core being loaded. Silences the engine and releases
    // the audio pins; a byte from a later core brings it back. A no-op if MIDI never started.
    void midi_stop(void);
#ifdef __cplusplus
}
#endif
#endif //MIDI_RECEIVER_H