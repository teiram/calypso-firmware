add_library(tzx STATIC
	TzxService.cxx
)

target_include_directories(tzx PRIVATE 
	.
	../core
)

target_link_libraries(tzx 
	pico_stdio_uart
	pico_stdlib
)
