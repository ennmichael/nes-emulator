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
                cpu.ram.write_byte(program_start + i, byte);
        }
}

}

int main(int, char**)
{
        Emulator::CPU cpu {
                .pc = program_start
        };

        Emulator::Bytes program {
                0xA9, 0xFF, 0x69, 0x01
        };

        write_program(cpu, program);
        cpu.execute_instruction(cpu.ram);

        using namespace Emulator;

        std::cout << Emulator::Utils::twos_complement(0b11111111) << '\n';
        std::cout << Emulator::Utils::sign_bit(0b11111111) << '\n';
        std::cout << ~(static_cast<Byte>(0b11111111) & ~static_cast<Byte>(0x80)) << '\n';
}
