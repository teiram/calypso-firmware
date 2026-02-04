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
    tzx
    mist
    core
    tinyusb_host
    tinyusb_board
)

pico_enable_stdio_usb(${PROGRAM_NAME} 0)
pico_enable_stdio_uart(${PROGRAM_NAME} 1)

pico_add_extra_outputs(${PROGRAM_NAME})

add_subdirectory(core)
add_subdirectory(mist)
add_subdirectory(tzx)
