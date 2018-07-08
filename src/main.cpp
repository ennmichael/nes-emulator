#include "cpu.h"
#include <iostream>

using namespace std::string_literals;

namespace {

unsigned constexpr program_start = 0x600u;

// TODO Delete me
void write_program(Emulator::CPU& cpu, Emulator::Bytes const& program) noexcept
{
        for (unsigned i = 0; i < program.size(); ++i) {
                auto const byte = program[i];
                cpu.memory->write_byte(program_start + i, byte);
        }
}

}

int main(int, char**)
{
        Emulator::CPU cpu {
                .memory = std::make_unique<Emulator::CPU::RAM>(),
                .pc = program_start
        };

        Emulator::Bytes program {
                0xA9, 0xFF, 0x85, 0x00, 0x4C, 0x0E, 0x06, 0xE6, 
                0x00, 0xE6, 0x01, 0xE6, 0x02, 0x60, 0x08, 0x20,
                0x07, 0x06, 0x08, 0x20, 0x07, 0x06, 0x08, 0x20, 
                0x1A, 0x06, 0xEA
        };

        write_program(cpu, program);
        cpu.execute_program(program.size());
}

