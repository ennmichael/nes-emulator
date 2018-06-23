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

std::size_t CPU::Memory::read_address(std::size_t pointer) const noexcept
{
        Byte const low = read(pointer);
        Byte const high = read(pointer + 1);
        return Utils::combine_little_endian(low, high);
}

CPU::CPU(Cartridge& cartridge, PPU& ppu) noexcept
        : memory(cartridge, ppu)
{}

void CPU::execute(Bytes const& program)
{
        // TODO
}

bool CPU::status(std::size_t flag) const noexcept
{
        return p.test(flag);
}

void CPU::status(std::size_t flag, bool value) noexcept
{
        p.set(flag, value);
}

void CPU::update_carry_flag(unsigned operation_result) noexcept
{
        status(carry_flag, operation_result >= byte_max);
}

void CPU::update_zero_flag(unsigned operation_result) noexcept
{
        status(zero_flag, operation_result == 0);
}

void CPU::update_overflow_flag(unsigned operation_result) noexcept
{
        status(overflow_flag, (operation_result << 7) != (a << 7));
}

void CPU::update_negative_flag(unsigned operation_result) noexcept
{
        status(overflow_flag, (operation_result << 7) == 1);
}

namespace
{

// TODO Move this to Utils::
template <class F, class... Args>
bool constexpr returns_void =
        std::is_same_v<std::invoke_result_t<F, Args...>, void>;

// Perhaps the instructions can be genralized, if they all:
// 1) Calculate a result
// 2) update the flags
// 3) return the result (perhaps modified)

void adc(CPU& cpu, Byte operand) noexcept
{
        unsigned const result = static_cast<unsigned>(cpu.a) +
                                static_cast<unsigned>(operand) +
                                static_cast<unsigned>(cpu.status(CPU::carry_flag));

        cpu.update_carry_flag(result);
        cpu.update_zero_flag(result);
        cpu.update_overflow_flag(result);
        cpu.update_negative_flag(result);

        cpu.a = (result < byte_max) ? result : result - byte_max;
}

void bitwise_and(CPU& cpu, Byte operand) noexcept
{
        unsigned const result = cpu.a & operand;

        cpu.update_negative_flag(result);
        cpu.update_zero_flag(result);

        cpu.a = result;
}

// FIXME This is a terrible name
std::size_t address_operand(std::size_t pc, Bytes const& program)
{
        Byte const lower = program[pc + 1];
        Byte const higher = program[pc + 2];
        return Utils::combine_little_endian(lower, higher);
}

// Used to avoid instantiating invalid code
template <class OperationCallback>
void operate_on_memory_nonvoid(OperationCallback const& operation,
                               Byte operand,
                               CPU& cpu,
                               std::size_t address)
{
        Byte const result = operation(cpu, operand);
        cpu.memory.write(address, result);
}

template <class OperationCallback>
void operate_on_memory(OperationCallback const& operation,
                       CPU& cpu, std::size_t address)
{
        Byte const operand = cpu.memory.read(address);

        if constexpr (returns_void<OperationCallback, CPU&, std::size_t>)
                operation(cpu, operand);
        else
                operate_on_memory_nonvoid(operation, operand, cpu, address);
}

template <class OperationCallback, class OffsetCallback>
Instruction zero_page_offset(OperationCallback operation,
                             OffsetCallback offset)
{
        return [operation, offset](CPU& cpu, Bytes const& program)
        {
                Byte const base_address = program[cpu.pc + 1];
                std::size_t const address =
                        static_cast<std::size_t>(base_address) + offset(cpu);
                operate_on_memory(operation, cpu, address);
                cpu.pc += 2;
        };
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
Instruction absolute_offset(OperationCallback operation,
                            OffsetCallback offset)
{
        return [operation, offset](CPU& cpu, Bytes const& program)
        {
                std::size_t const address = address_operand(cpu.pc, program);
                operate_on_memory(operation, cpu, address);
        };
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
        return [operation](CPU& cpu, Bytes const& program)
        {
                std::size_t const pointer = address_operand(cpu.pc, program);
                std::size_t const address = cpu.memory.read(pointer);
                operate_on_memory(operation, cpu, address);
                cpu.pc += 3;
        };
}

template <class OperationCallback>
Instruction implied(OperationCallback const& operation)
{
        return [operation](CPU& cpu, Bytes const&)
        {
                operation(cpu);
                cpu.pc += 1;
        };
}

template <class OperationCallback>
Instruction accumulator(OperationCallback const& operation)
{
        return [operation](CPU& cpu, Bytes const&)
        {
                cpu.a = operation(cpu, cpu.a);
                cpu.pc += 1;
        };
}

template <class OperationCallback>
Instruction immediate(OperationCallback operation)
{
        return [operation](CPU& cpu, Bytes const& program)
        {
                Byte const operand = program[cpu.pc + 1];
                operation(cpu, operand);
                cpu.pc += 2;
        };
}

template <class OperationCallback>
Instruction relative(OperationCallback const& operation)
{
        return [operation](CPU& cpu, Bytes const& program)
        {
                Byte const operand = program[cpu.pc + 1];
                int const delta = Utils::twos_complement(operation(cpu, operand));
                cpu.pc += 2 + delta;
        };
}

template <class OperationCallback>
Instruction indirect_x(OperationCallback const& operation) // Indexed indirect
{
        return [operation](CPU& cpu, Bytes const& program)
        {
                std::size_t const pointer = program[cpu.pc + 1] + cpu.x;
                std::size_t const address = cpu.memory.read_address(pointer);
                operate_on_memory(operation, cpu, address);
                cpu.pc += 2;
        };
}

template <class OperationCallback>
Instruction indirect_y(OperationCallback const& operation) // Indirect indexed
{
        return [operation](CPU& cpu, Bytes const& program)
        {
                std::size_t const pointer = program[cpu.pc + 1];
                std::size_t const address =
                        cpu.memory.read_address(pointer) + cpu.y;
                operate_on_memory(operation, cpu, address);
                cpu.pc += 2;
        };
}

}

Instruction translate_opcode(Byte opcode)
{
        switch (opcode) {
                // TODO Auto-generate this
        }
}

}

