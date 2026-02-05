add_library(tape STATIC
    TapeService.cxx
    PulseRenderer.cxx
    TzxTapeParser.cxx
)

target_include_directories(tape PRIVATE 
	.
	../core
)

target_link_libraries(tape 
	pico_stdio_uart
	pico_stdlib
)
