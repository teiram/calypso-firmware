add_library(core STATIC
	CRC.cxx
	SPIDevice.cxx
	PS2Device.cxx
	PS2Mouse.cxx
	PS2Keyboard.cxx
	JTAG.cxx
	SPISDCard.cxx
	Service.cxx
	DB9Joystick.cxx
	USBService.cxx
	Util.cxx
	Button.cxx
	AudioTarget.cxx
	I2SAudioTarget.cxx
	MIDIStateMachine.cxx
	MIDIService.cxx
)

target_include_directories(core PRIVATE .)
pico_generate_pio_header(core ${CMAKE_CURRENT_LIST_DIR}/ps2.pio)

target_compile_options(core PUBLIC
  "-DPICO_AUDIO_PIO=1"
)

target_link_libraries(core 
	pico_stdio_uart
	pico_stdlib
	hardware_spi
	hardware_pio
	tinyusb_board
	tinyusb_host
	pico_audio
	pico_audio_i2s
)
