#include "cpu.h"
#include <utility>

namespace Emulator {

CPU::Memory::Memory(Cartridge& cartridge, PPU& ppu) noexcept
        : cartridge_(&cartridge)
        , ppu_(&ppu)
{}

void CPU::Memory::write(std::size_t address, Byte byte) noexcept
{
        ram_[address % internal_ram_size] = byte;
}

Byte CPU::Memory::read(std::size_t address) const noexcept
{
        return ram_[address % internal_ram_size];
}

CPU::CPU(Cartridge& cartridge, PPU& ppu, InstructionSet instruction_set) noexcept
        : memory_(cartridge, ppu)
        , instruction_set_(std::move(instruction_set))
{}

CPU::CPU(Cartridge& cartridge, PPU& ppu)
        : CPU(cartrigge, ppu, standard_instruction_set())
{}

void CPU::execute(Bytes const& program)
{

}

unsigned CPU::pc() const noexcept
{
        return pc_;
}

Byte CPU::sp() const noexcept
{
        return sp_;
}

Byte CPU::a() const noexcept
{
        return a_;
}

Byte CPU::x() const noexcept
{
        return x_;
}

Byte CPU::y() const noexcept
{
        return y_;
}

bool CPU::status(std::size_t flag) const noexcept
{
        return p_.test(flag);
}

void CPU::status(std::size_t flag, bool value) noexcept
{
        p_.set(flag, value);
}

void CPU::modify_carry_flag(unsigned operation_result) noexcept
{
        status(carry_flag, operation_result >= byte_max);
}

void CPU::modify_zero_flag(unsigned operation_result) noexcept
{
        status(zero_flag, operation_result == 0);
}

void CPU::modify_overflow_flag(unsigned operation_result) noexcept
{
        status(overflow_flag, (operation_result << 7) != (a_ << 7));
}

void CPU::modify_negative_flag(unsigned operation_result) noexcept
{
        status(overflow_flag, (operation_result << 7) == 1);
}

void CPU::adc(CPU& cpu, Byte operand) noexcept
{
        unsigned const result = static_cast<unsigned>(cpu.registers_.a) +
                                static_cast<unsigned>(value) +
                                static_cast<unsigned>(cpu.status(carry_flag));

        cpu.modify_carry_flag(result);
        cpu.modify_zero_flag(result);
        cpu.modify_overflow_flag(result);
        cpu.modify_negative_flag(result);

        cpu.a_ = (result < byte_max) ? result : result - byte_max;
}

void CPU::and(CPU& cpu, Byte operand) noexcept
{
        
}

auto CPU::standard_instruction_set() -> InstructionSet
{
        auto const adc =
        [](CPU& cpu, Byte value)
        {
                
        };

        auto const and =
        [](i)

        return {
                {

                }
        };
}

}

