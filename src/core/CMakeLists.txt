add_library(core STATIC
	CRC.cxx
	SPIDevice.cxx
	PS2Device.cxx
	PS2Mouse.cxx
	PS2Keyboard.cxx
	JTAG.cxx
	SPISDCard.cxx
	Service.cxx
	Joystick.cxx
	USBService.cxx
	Util.cxx
	Button.cxx
)

target_include_directories(core PRIVATE .)
pico_generate_pio_header(core ${CMAKE_CURRENT_LIST_DIR}/ps2.pio)

target_link_libraries(core pico_stdio_uart pico_stdlib hardware_spi hardware_pio tinyusb_board tinyusb_host)
