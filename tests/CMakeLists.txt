add_executable(tests tests.cpp utils_tests.cpp memory_tests.cpp cartridge_tests.cpp cpu_tests.cpp joypad_tests.cpp ppu_tests.cpp)
target_link_libraries(tests nes-emulator-lib)
add_compile_options(tests)

