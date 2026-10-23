add_library(midi STATIC
	AudioTarget.cxx
	I2SAudioTarget.cxx
	MidiFileParser.cxx
	MidiStreamParser.cxx
	MidiPlayerService.cxx
	MIDIService.cxx
	SquareWavePlayer.cxx
	WaveTablePlayer.cxx
	FixedReverb.cxx
)

# The mixer is a fixed point loop that runs tens of thousands of times a second;
# it has nothing left to debug at -Og. This must precede the library.
set_source_files_properties(WaveTablePlayer.cxx PROPERTIES COMPILE_OPTIONS "-O2")

# WaveTablePlayer.cxx #includes the generated tables (drums_k2.c,
# waveforms_flr3_128.c, fx_samples.c) rather than compiling them: they are data,
# and compiling one separately would define each const table a second time with
# external linkage.

target_include_directories(midi PRIVATE 
	.
	../core
)

target_compile_options(midi PUBLIC
  "-DPICO_AUDIO_PIO=1"
)

target_link_libraries(midi 
	pico_stdio_uart
	pico_stdlib
	pico_audio
	pico_audio_i2s
	pico_rand
)
