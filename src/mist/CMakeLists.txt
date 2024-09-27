set (MIST_FW_PATH ../../libs/mist-firmware)

#Assumed by mist firmware
add_compile_options(-fsigned-char)

add_library(mist STATIC
	db9.cxx
	mmc.cxx
	spi.cxx
	fpga.cxx
	ps2keyboard.cxx
	ps2mouse.cxx
	compat.c
	usbdev.c
	${MIST_FW_PATH}/arc_file.c
	${MIST_FW_PATH}/archie.c
	${MIST_FW_PATH}/boot.c
	${MIST_FW_PATH}/cdc_control.c
	${MIST_FW_PATH}/config.c
	${MIST_FW_PATH}/cue_parser.c
	${MIST_FW_PATH}/data_io.c
	${MIST_FW_PATH}/fat_compat.c
	${MIST_FW_PATH}/fdd.c
	${MIST_FW_PATH}/font.c
	${MIST_FW_PATH}/fpga.c
	${MIST_FW_PATH}/hdd.c
	${MIST_FW_PATH}/idxfile.c
	${MIST_FW_PATH}/ikbd.c
	${MIST_FW_PATH}/ini_parser.c
	${MIST_FW_PATH}/menu-8bit.c
	${MIST_FW_PATH}/menu.c
	${MIST_FW_PATH}/menu-minimig.c
	${MIST_FW_PATH}/mist_cfg.c
	${MIST_FW_PATH}/neocd.c
	${MIST_FW_PATH}/osd.c
	${MIST_FW_PATH}/pcecd.c
	${MIST_FW_PATH}/settings.c
	${MIST_FW_PATH}/state.c
	${MIST_FW_PATH}/swap.c
	${MIST_FW_PATH}/tos.c
	${MIST_FW_PATH}/user_io.c
	${MIST_FW_PATH}/utils.c
	${MIST_FW_PATH}/xmodem.c
	${MIST_FW_PATH}/FatFs/diskio.c
	${MIST_FW_PATH}/FatFs/ff.c
	${MIST_FW_PATH}/FatFs/ffsystem.c
	${MIST_FW_PATH}/FatFs/ffunicode.c
	${MIST_FW_PATH}/usb/joymapping.c
	${MIST_FW_PATH}/usb/hidparser.c
	${MIST_FW_PATH}/usb/hid.c
	${MIST_FW_PATH}/usb/joystick.c
	${MIST_FW_PATH}/usb/xboxusb.c
	${MIST_FW_PATH}/usb/timer.c
	${MIST_FW_PATH}/usb/usbdebug.c
	MistUSBKeyboardHandler.cxx
	MistUSBMouseHandler.cxx
	MistService.cxx)
target_compile_options(mist PRIVATE
  "-DVDATE=\"\""
  "-DMIST"
  "-DMIST_ROOT=\"/\""
  "-DCOREEXT=\"RBF\""
  "-DCOREEXTOTHER=\"NP1\""
  "-DCONFIG_ARCH_ARM"
  "-Duprintf=printf"
)

target_include_directories(mist PRIVATE 
	.
	../core
	../../libs
	../../libs/mist-firmware
	../../libs/mist-firmware/arch
	../../libs/mist-firmware/usb
	usbhost)
target_link_libraries(mist pico_stdio_uart pico_stdlib hardware_spi hardware_pio tinyusb_board tinyusb_host)
