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

bool CPU::status(std::size_t flag) const noexcept
{
        return p.test(flag);
}

void CPU::status(std::size_t flag, bool value) noexcept
{
        p.set(flag, value);
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

namespace
{

void adc(CPU& cpu, Byte operand) noexcept
{
        unsigned const result = static_cast<unsigned>(cpu.a) +
                                static_cast<unsigned>(value) +
                                static_cast<unsigned>(cpu.status(carry_flag));

        cpu.modify_carry_flag(result);
        cpu.modify_zero_flag(result);
        cpu.modify_overflow_flag(result);
        cpu.modify_negative_flag(result);

        cpu.a = (result < byte_max) ? result : result - byte_max;
}

void and(CPU& cpu, Byte operand) noexcept
{
        unsigned const result = cpu.a & operand;

        cpu.modify_negative_flag(result);
        cpu.modify_zero_flag(result);

        cpu.a = result;
}

template <class OperationCallback, class OffsetCallback>
Instruction zero_page_offset(OperationCallback const& operation,
                                    OffsetCallback const& offset)
{
        return [operation](CPU& cpu, Byte base_address)
        {
                std::size_t const address =
                        static_cast<std::size_t>(base_address) + offset(cpu);
                Byte const operand = cpu.memory.read(address);
                return operation(cpu, operand);
        }
}

template <class OperationCallback>
Instruction zero_page(OperationCallback const& operation)
{
        return zero_page_offset(operation,
                                [](CPU const&) { return 0; });
}

template <class OperationCallback>
Instruction zero_page_x(OperationCallback const& operation)
{
        return zero_page_offset(operation,
                                [](CPU const& cpu) { return cpu.x; });
}

template <class OperationCallback>
Instruction zero_page_y(OperationCallback const& operation)
{
        return zero_page_offset(operation,
                                [](CPU const& cpu) { return cpu.y; });
}

template <class OperationCallback, class OffsetCallback>
Instruction absolute_offset(OperationCallback const& operation,
                            OffsetCallback const& offset)
{
        // This should be abstracted away
        switch (Sdl::endianness) {
                case Sdl::Endianness::little: break;
                case Sdl::Endianness::big: break;
        }
}

template <class OperationCallback>
Instruction absolute(OperationCallback const& operation)
{
        return absolute_offset(operation,
                               [](CPU const&) { return 0; });
}

template <class OperationCallback>
Instruction absolute_x(OperationCallback const& operation)
{
        return absolute_offset(operation,
                               [](CPU const& cpu) { return cpu.x; });
}

template <class OperationCallback>
Instruction absolute_y(OperationCallback const& operation)
{
        return absolute_offset(operation,
                               [](CPU const& cpu) { return cpu.y; });
}

template <class OperationCallback>
Instruction indirect(OperationCallback const& operation)
{
}

template <class OperationCallback>
Instruction implied(OperationCallback const& operation)
{
}

template <class OperationCallback>
Instructon accumulator(OperationCallback const& operation)
{
}

template <class OperationCallback>
Instruction immediate(OperationCallback const& operation)
{
}

template <class OperationCallback>
Instruction relative(OperationCallback const& operation)
{
}

template <class OperationCallback>
Instruction indirect_x(OperationCallback const& operation)
{
}

template <class OperationCallback>
Instruction indirect_y(OperationCallback const& operation) // Indirect indexed
{
}

auto in_place()

}

Instruction translate_opcode(Byte opcode)
{
        switch (opcode) {
                case 0x69: return immediate(adc);
                case 0x65: return zero_page(adc);
                case 0x75: return zero_page_x(adc);
                case 0x6D: return absolute(adc);
        }
}

}

