# Calypso Firmware

This firmware supports the middleboard [Calypso](https://github.com/teiram/calypso-cyc1000-board) featuring Mist compatibility.

In order to encourage experimentation and alternate approaches, the firmware is organized in the following way:
- Folder src/core. Provides the C++ foundations of the firmware. A sort of hardware abstraction layer that can be used to easily manage the rp2040 peripherals.
- Folder src/mist. Provides the Mist adaptation layer so that the oficial mist-firmware can find all the required dependencies and run. The mist firmware lifecycle is implemented in the class `calypso::MistService`
- Folder src/midi. Provides the MIDI and audio path. `calypso::MIDIService` owns the I2S audio target and feeds the incoming MIDI byte stream to a `Player`. `Player` is a selectable synthesiser engine; the firmware runs `WaveTablePlayer` (a per-key wavetable palette derived from the FluidR3_GM soundfont, a small set of recorded sample voices for the programs a cycle cannot represent, and an embedded drum kit, all compiled into the image), while the legacy `SquareWavePlayer` is kept alongside it. The engine needs the core overclocked to 250 MHz (see `src/main.cxx`). The same folder holds the MIDI file player: `calypso::MidiFileParser` reads a Standard MIDI File (format 0 or 1, merged tracks, with tempo), and `calypso::MidiPlayerService` replays it over the wall clock into the MIDI service. The Mist-side OSD page parser for it lives in `src/mist/midi_processor.cxx` (page plugin id `MID`).
- Folder libs. 3rd party libraries or modules are expected to be added here (so far only mist-firmware is provided)
- src/main.cxx. This is the core of the firmware, where the different componentes are instantiated. Elements with lifecycle can be implemented as `calypso::Service` subclasses (like the existing MistService or USBService), registered in the Service repository and main will take care of their initialization and updates.

The official Mist firmware is included as a submodule, it's the official unmodified version that can be found in https://github.com/mist-devel/mist-firmware

# Compiling from sources
## Requirements
- Pico SDK 2 properly installed and configured. If you are able to build the pico examples you are good to go. Please refer to [this page](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
- It should work regardless of your operating system. I'm using Linux Mint 21.3
## Procedure
- Clone this repository
```
  git clone https://github.com/teiram/calypso-firmware
```
- Create an empty build directory (you can do it inside the repository itself since `build` is in .gitignore blacklist but another directory outside of the repository folder is also okey
- Create the makefiles with the cmake utility, providing as argument the source location (the location of the top CMakeLists.txt file). For instance if you created the build directory inside the repository itself, it would be like:
```
    cd build
    cmake ..
```   
- Compile the firmware. Just run make inside the build directory
- Install the firmware on your rp2040. If you have another pico as SWD host connected to the pico on the calypso board, and a working openocd tool, you can just run:
```
    make upload
```
  otherwise you can also upload the firmware via USB (press boot and reset on the USB connected RP2040 and drop the file on the opening Mass Storage device) using the generated file
```
    build/src/calypso-firmware.uf2
```

# Updating the firmware from the SD card

The firmware can update itself from a file on the SD card, the same way the
original mist-firmware does it on the MiST: the update is triggered from the
on-screen menu (`Update FIRMWARE`), no USB or SWD host is needed.

## Preparing the update file

The build tags `build/src/calypso-firmware.uf2` with a metadata block
(version, build date, git hash and a CRC32 of the whole image, appended by
`tools/uf2meta.py`). Copy that file to the root of the SD card and rename it:

```
    FIRMWARE.UPG
```

Only a UF2 with a valid metadata block is accepted; a plain UF2 from another
source is rejected with `Invalid update file`. The same file can still be
dropped on the USB drive in BOOTSEL mode as usual - the metadata block is a
regular UF2 block and is simply programmed into the last bytes of the flash.

## Updating

With `FIRMWARE.UPG` on the SD card root:

- The `About` page shows both versions: `ARM s/w ver.` (running firmware) and
  `FILE s/w ver.` (the update file).
- The `Update FIRMWARE` menu item validates the file (magic, family, block
  layout and payload CRC32) and asks for confirmation.

After confirmation the update runs in two phases, takes a few minutes and the
disk LED blinks while it works:

1. The new image is streamed into a staging area in the upper half of the
   flash and verified against the metadata CRC.
2. The staged image is copied over the firmware from a RAM-resident routine
   and the device reboots into the new version. Do not power off during this
   phase: a running core may stall briefly until the reboot.

If the update is interrupted (power loss), the RP2040 bootrom will not find a
valid image and falls back to BOOTSEL USB mass storage, so the board can
always be recovered by dragging a firmware UF2 (or any earlier release) onto
it.

## Updating from the menu while development

For testing the update flow without touching the flash, build with
`-DUPDATE_DRY_RUN=ON`: the update file is validated and staged, the final
programming step is skipped and a message is logged on the UART console.

