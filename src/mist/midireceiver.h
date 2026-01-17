#ifndef MIDI_RECEIVER_H
#define MIDI_RECEIVER_H

#ifdef __cplusplus
extern "C" {
#endif
    #include <inttypes.h>
    
    void midi_byte_processor(uint8_t value);
#ifdef __cplusplus
}
#endif
#endif //MIDI_RECEIVER_H