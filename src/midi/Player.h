#ifndef PLAYER_H
#define PLAYER_H

#include <cinttypes>

// One selectable audio engine. MIDIService drives whichever Player it is given,
// and a Player owns its own audio rendering (synthTask pulls a buffer from the
// AudioTarget, fills it and gives it back).
//
// WaveTablePlayer is the engine the firmware runs. SquareWavePlayer is kept
// buildable as the alternative engine; selecting between them is a matter of
// constructing the wanted one and handing it to MIDIService (there is no
// runtime selector yet).
class Player {
public:
    // Called once, after the AudioTarget is initialised and before the first
    // synthTask(). Returns false if the engine could not start.
    virtual bool init() = 0;
    virtual void playNote(uint8_t channel, uint8_t pitch, uint8_t velocity) = 0;
    virtual void stopNote(uint8_t channel, uint8_t pitch) = 0;
    // Silence every sounding voice, as a MIDI note-off would but without needing
    // a note-off for each key. The engine's own note table is authoritative, so
    // this lives here rather than being tracked separately by the caller: it is
    // what the idle watchdog and the FPGA reload hook call when a stream stops
    // mid-note and the note-off that would have released a key never arrives.
    virtual void allNotesOff() = 0;
    virtual void changeProgram(__attribute__((__unused__)) uint8_t channel,
        __attribute__((__unused__)) uint8_t program) {};
    virtual void setVolume(__attribute__((__unused__)) uint8_t channel,
        __attribute__((__unused__)) uint8_t volume) {};
    virtual void setPan(__attribute__((__unused__)) uint8_t channel,
        __attribute__((__unused__)) uint8_t pan) {};
    virtual void setPitchBending(__attribute__((__unused__)) uint8_t channel,
        __attribute__((__unused__)) uint16_t bending) {};
    virtual void synthTask() = 0;

    virtual void showStats() = 0;

    // Producer pool geometry this engine wants from the AudioTarget. The
    // wavetable mixer quantises a note start to a block, so it wants short
    // blocks; the square engine is indifferent. Both keep the same total slack
    // (count * samples frames), so the choice only moves the delivery step.
    virtual uint16_t audioBufferCount() const = 0;
    virtual uint16_t audioBufferSamples() const = 0;
};

#endif /* PLAYER_H */
