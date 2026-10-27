cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PROGRAM_NAME calypso-firmware)

#So far needed, something odd happens with optimization
set(CMAKE_BUILD_TYPE RELEASE)

#set(LOG 3)

set(FVERSION "0.22")

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(${PROGRAM_NAME})

# The Calypso board carries a VCC-GND RP2040 module with a 16 MB (W25Q128) part.
# The SDK sizes the linker flash region from this variable (default 2 MB for the
# PICO board), and the wavetable palette plus the sample voices push the image
# past 2 MB, so without it nothing can be placed above the first 2 MB.
set(PICO_FLASH_SIZE_BYTES 16777216 CACHE STRING "RP2040 XIP flash size (16 MB part)")

# initialize the sdk
pico_sdk_init()


add_subdirectory(src)

add_custom_target(upload openocd -f interface/cmsis-dap.cfg  -f target/rp2040.cfg -c \"adapter speed 5000\; program src/calypso-firmware.elf verify\;reset\;shutdown\")
