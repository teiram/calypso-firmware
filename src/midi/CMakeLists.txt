add_library(midi STATIC
	AudioTarget.cxx
	I2SAudioTarget.cxx
	MIDIStateMachine.cxx
	MIDIService.cxx
)

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
)
