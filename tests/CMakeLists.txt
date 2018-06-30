# TODO This should be optionally compiled
add_executable(tests tests.cpp utils_tests.cpp cpu_memory_tests.cpp cartridge_tests.cpp cpu_6502_tests.cpp)
target_link_libraries(tests nes-emulator-lib)
add_compile_options(tests)

