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
                0xA9, 0x02, 0x85, 0x04, 0x8D, 0x00, 0x02, 0xA9,
                0x05, 0x38, 0xE9, 0x06, 0x48, 0x08, 0xE5, 0x04
        };

        write_program(cpu, program);
        cpu.execute_program(program.size());

        std::cout << "0x" << std::hex << static_cast<unsigned>(cpu.a) << '\n';
        std::cout << cpu.p << '\n';
}

