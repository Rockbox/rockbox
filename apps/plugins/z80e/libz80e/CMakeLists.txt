cmake_minimum_required(VERSION 2.8)
project (z80e_objects)

FILE(GLOB Hardware src/ti/hardware/*.c)
FILE(GLOB Commands src/debugger/commands/*.c)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/z80e)

if(DEFINED EMSCRIPTEN)
add_compile_options(-Wno-warn-absolute-paths)
add_definitions(-DEMSCRIPTEN)
endif()

add_library(z80e_objects OBJECT
	src/core/cpu.c
	src/core/registers.c
	src/debugger/debugger.c
	src/debugger/hooks.c
	src/disassembler/disassemble.c
	src/ti/asic.c
	src/ti/memory.c
	src/log/log.c
	src/runloop/runloop.c
	${Hardware}
	${Commands}
)
