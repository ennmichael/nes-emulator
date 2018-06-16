cmake_minimum_required(VERSION 3.0.1)
project(nes-emulator)

add_executable(nes-emulator src/main.cpp src/sdl++.cpp)
target_compile_options(nes-emulator PRIVATE "-Wall")
target_compile_options(nes-emulator PRIVATE "-Wextra")
target_compile_options(nes-emulator PRIVATE "-pedantic")
target_compile_options(nes-emulator PRIVATE "-std=c++17")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${nes-emulator_SOURCE_DIR}/cmake")

find_package(SDL2 REQUIRED)

if (SDL2_FOUND)
        target_include_directories(nes-emulator PRIVATE ${SDL2_INCLUDE_DIR})
        target_link_libraries(nes-emulator PRIVATE ${SDL2_LIBRARY})
else()
        message(FATAL_ERROR "SDL2 not found. Try running sudo apt-get install libsdl2-dev.")
endif()

