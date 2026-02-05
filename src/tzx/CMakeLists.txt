add_library(tzx STATIC
    TapeService.cxx
    PulseRenderer.cxx
    TzxTapeParser.cxx
)

target_include_directories(tzx PRIVATE 
	.
	../core
)

target_link_libraries(tzx 
	pico_stdio_uart
	pico_stdlib
)
