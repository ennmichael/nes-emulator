cmake_minimum_required(VERSION 3.0.1)
project(nes-emulator)

macro(add_compile_options target)
        target_compile_options(${target} PRIVATE "-Wall")
        target_compile_options(${target} PRIVATE "-Wextra")
        target_compile_options(${target} PRIVATE "-std=c++17")
endmacro()

add_library(nes-emulator-lib src/sdl++.cpp src/cpu.cpp src/ppu.cpp src/cartridge.cpp src/utils.cpp src/instruction_set.cpp src/mem.cpp)
add_compile_options(nes-emulator-lib)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${nes-emulator_SOURCE_DIR}/cmake")

find_package(SDL2 REQUIRED)

if (SDL2_FOUND)
        target_include_directories(nes-emulator-lib PRIVATE ${SDL2_INCLUDE_DIR})
        target_link_libraries(nes-emulator-lib PRIVATE ${SDL2_LIBRARY})
else()
        message(FATAL_ERROR "SDL2 not found. Try running sudo apt-get install libsdl2-dev.")
endif()


add_executable(nes-emulator src/main.cpp)
target_link_libraries(nes-emulator nes-emulator-lib)
add_compile_options(nes-emulator)

add_subdirectory(tests)

