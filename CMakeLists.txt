cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PROGRAM_NAME calypso-firmware)

#So far needed, something odd happens with optimization
set(CMAKE_BUILD_TYPE RELEASE)

#set(LOG 3)

set(FVERSION "0.18")

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(${PROGRAM_NAME})


# initialize the sdk
pico_sdk_init()


add_subdirectory(src)

add_custom_target(upload openocd -f interface/cmsis-dap.cfg  -f target/rp2040.cfg -c \"adapter speed 5000\; program src/calypso-firmware.elf verify\;reset\;shutdown\")
