set(PROGRAM_NAME calypso-firmware)

set(FAMILY rp2040)
set(BOARD pico_sdk)

add_executable(${PROGRAM_NAME} 
    main.cxx 
)

target_include_directories(${PROGRAM_NAME} PRIVATE core mist/usbhost)

#To enable tinyusb logs
#Maybe better enable with some configuration
#target_compile_options(${PROGRAM_NAME} PRIVATE
#  "-DLOG=3"
#)


target_link_libraries(${PROGRAM_NAME} PRIVATE
    pico_stdlib
    pico_stdio_uart
    hardware_pio
    hardware_spi
    pico_multicore
    tape
    midi
    mist
    core
    tinyusb_host
    tinyusb_board
)

pico_enable_stdio_usb(${PROGRAM_NAME} 0)
pico_enable_stdio_uart(${PROGRAM_NAME} 1)

pico_add_extra_outputs(${PROGRAM_NAME})

# Append the Calypso metadata block (version, build date, git hash, payload
# CRC32) to the generated UF2, right after picotool produced it. The on-device
# updater refuses to flash a /FIRMWARE.UPG file without it.
# Releases are named CLP-<FVERSION> (see version[] in src/mist/compat.c), and
# the metadata block carries that display name.
string(TIMESTAMP FW_UPDATE_DATE "%Y%m%d")
execute_process(COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    OUTPUT_VARIABLE FW_UPDATE_GIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if (NOT FW_UPDATE_GIT)
    set(FW_UPDATE_GIT "unknown")
endif()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/uf2meta.py
        --version "CLP-${FVERSION}" --date "${FW_UPDATE_DATE}" --git "${FW_UPDATE_GIT}"
        ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_NAME}.uf2
    COMMENT "Tagging ${PROGRAM_NAME}.uf2 with firmware metadata")

add_subdirectory(core)
add_subdirectory(mist)
add_subdirectory(midi)
add_subdirectory(tape)
