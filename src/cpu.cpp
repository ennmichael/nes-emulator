#include "cpu.h"
#include <utility>
#include <string>

using namespace std::string_literals;

namespace Emulator {

UnknownOpcode::UnknownOpcode(Byte opcode) noexcept
        : runtime_error("Unknown opcode "s + std::to_string(opcode))
{}

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

Instruction CPU::translate_opcode(Byte opcode)
{
        // This code was automatically generated
        // using scripts/scrape_opcodes.py.
        switch (opcode) {
                case 0x69: return immediate(adc)
                case 0x65: return zero_page(adc)
                case 0x75: return zero_page_x(adc)
                case 0x6D: return absolute(adc)
                case 0x7D: return absolute_x(adc)
                case 0x79: return absolute_y(adc)
                case 0x61: return indirect_x(adc)
                case 0x71: return indirect_y(adc)
                case 0x29: return immediate(bitwise_and)
                case 0x25: return zero_page(bitwise_and)
                case 0x35: return zero_page_x(bitwise_and)
                case 0x2D: return absolute(bitwise_and)
                case 0x3D: return absolute_x(bitwise_and)
                case 0x39: return absolute_y(bitwise_and)
                case 0x21: return indirect_x(bitwise_and)
                case 0x31: return indirect_y(bitwise_and)
                case 0x0A: return accumulator(asl)
                case 0x06: return zero_page(asl)
                case 0x16: return zero_page_x(asl)
                case 0x0E: return absolute(asl)
                case 0x1E: return absolute_x(asl)
                case 0x90: return relative(bcc)
                case 0xB0: return relative(bcs)
                case 0xF0: return relative(beq)
                case 0x24: return zero_page(bit)
                case 0x2C: return absolute(bit)
                case 0x30: return relative(bmi)
                case 0xD0: return relative(bne)
                case 0x10: return relative(bpl)
                case 0x00: return implied(brk)
                case 0x50: return relative(bvc)
                case 0x70: return relative(bvs)
                case 0x18: return implied(clc)
                case 0xD8: return implied(cld)
                case 0x58: return implied(cli)
                case 0xB8: return implied(clv)
                case 0xC9: return immediate(cmp)
                case 0xC5: return zero_page(cmp)
                case 0xD5: return zero_page_x(cmp)
                case 0xCD: return absolute(cmp)
                case 0xDD: return absolute_x(cmp)
                case 0xD9: return absolute_y(cmp)
                case 0xC1: return indirect_x(cmp)
                case 0xD1: return indirect_y(cmp)
                case 0xE0: return immediate(cpx)
                case 0xE4: return zero_page(cpx)
                case 0xEC: return absolute(cpx)
                case 0xC0: return immediate(cpy)
                case 0xC4: return zero_page(cpy)
                case 0xCC: return absolute(cpy)
                case 0xC6: return zero_page(dec)
                case 0xD6: return zero_page_x(dec)
                case 0xCE: return absolute(dec)
                case 0xDE: return absolute_x(dec)
                case 0xCA: return implied(dex)
                case 0x88: return implied(dey)
                case 0x49: return immediate(eor)
                case 0x45: return zero_page(eor)
                case 0x55: return zero_page_x(eor)
                case 0x4D: return absolute(eor)
                case 0x5D: return absolute_x(eor)
                case 0x59: return absolute_y(eor)
                case 0x41: return indirect_x(eor)
                case 0x51: return indirect_y(eor)
                case 0xE6: return zero_page(inc)
                case 0xF6: return zero_page_x(inc)
                case 0xEE: return absolute(inc)
                case 0xFE: return absolute_x(inc)
                case 0xE8: return implied(inx)
                case 0xC8: return implied(iny)
                case 0x4C: return absolute(jmp)
                case 0x6C: return indirect(jmp)
                case 0x20: return absolute(jsr)
                case 0xA9: return immediate(lda)
                case 0xA5: return zero_page(lda)
                case 0xB5: return zero_page_x(lda)
                case 0xAD: return absolute(lda)
                case 0xBD: return absolute_x(lda)
                case 0xB9: return absolute_y(lda)
                case 0xA1: return indirect_x(lda)
                case 0xB1: return indirect_y(lda)
                case 0xA2: return immediate(ldx)
                case 0xA6: return zero_page(ldx)
                case 0xB6: return zero_page_y(ldx)
                case 0xAE: return absolute(ldx)
                case 0xBE: return absolute_y(ldx)
                case 0xA0: return immediate(ldy)
                case 0xA4: return zero_page(ldy)
                case 0xB4: return zero_page_x(ldy)
                case 0xAC: return absolute(ldy)
                case 0xBC: return absolute_x(ldy)
                case 0x4A: return accumulator(lsr)
                case 0x46: return zero_page(lsr)
                case 0x56: return zero_page_x(lsr)
                case 0x4E: return absolute(lsr)
                case 0x5E: return absolute_x(lsr)
                case 0xEA: return implied(nop)
                case 0x09: return immediate(ora)
                case 0x05: return zero_page(ora)
                case 0x15: return zero_page_x(ora)
                case 0x0D: return absolute(ora)
                case 0x1D: return absolute_x(ora)
                case 0x19: return absolute_y(ora)
                case 0x01: return indirect_x(ora)
                case 0x11: return indirect_y(ora)
                case 0x48: return implied(pha)
                case 0x08: return implied(php)
                case 0x68: return implied(pla)
                case 0x28: return implied(plp)
                case 0x2A: return accumulator(rol)
                case 0x26: return zero_page(rol)
                case 0x36: return zero_page_x(rol)
                case 0x2E: return absolute(rol)
                case 0x3E: return absolute_x(rol)
                case 0x6A: return accumulator(ror)
                case 0x66: return zero_page(ror)
                case 0x76: return zero_page_x(ror)
                case 0x6E: return absolute(ror)
                case 0x7E: return absolute_x(ror)
                case 0x40: return implied(rti)
                case 0x60: return implied(rts)
                case 0xE9: return immediate(sbc)
                case 0xE5: return zero_page(sbc)
                case 0xF5: return zero_page_x(sbc)
                case 0xED: return absolute(sbc)
                case 0xFD: return absolute_x(sbc)
                case 0xF9: return absolute_y(sbc)
                case 0xE1: return indirect_x(sbc)
                case 0xF1: return indirect_y(sbc)
                case 0x38: return implied(sec)
                case 0xF8: return implied(sed)
                case 0x78: return implied(sei)
                case 0x85: return zero_page(sta)
                case 0x95: return zero_page_x(sta)
                case 0x8D: return absolute(sta)
                case 0x9D: return absolute_x(sta)
                case 0x99: return absolute_y(sta)
                case 0x81: return indirect_x(sta)
                case 0x91: return indirect_y(sta)
                case 0x86: return zero_page(stx)
                case 0x96: return zero_page_y(stx)
                case 0x8E: return absolute(stx)
                case 0x84: return zero_page(sty)
                case 0x94: return zero_page_x(sty)
                case 0x8C: return absolute(sty)
                case 0xAA: return implied(tax)
                case 0xA8: return implied(tay)
                case 0xBA: return implied(tsx)
                case 0x8A: return implied(txa)
                case 0x9A: return implied(txs)
                case 0x98: return implied(tya)
                default: throw UnknownOpcode(opcode);
        } 
}

namespace
{

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

Byte asl(CPU& cpu, Byte operand) noexcept
{
        unsigned const result = operand << 1;
        
        cpu.status(carry, operand & 0x80);
        cpu.update_zero_flag(result);
        cpu.update_negative_flag(result);

        return result;
}

void bit() noexcept
{
        
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

        if constexpr (Utils::returns_void<OperationCallback, CPU&, std::size_t>) {
                operation(cpu, operand);
        } else {
                operate_on_memory_nonvoid(operation, operand, cpu, address);
        }
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
                std::size_t const address
                        = Utils::combine_little_endian(progran[cpu.pc + 1],
                                                       program[cpu.pc + 2]);
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
                std::size_t const pointer =
                        Utils::combine_little_endian(program[cpu.pc + 1],
                                                     program[cpu.pc + 2]);
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

CPU::CPU(Cartridge& cartridge, PPU& ppu) noexcept
        : memory(cartridge, ppu)
{}

void CPU::execute(Bytes const& program)
{
        while (pc != program.size())
                execute_next_byte(program);
}

void CPU::execute_next_byte(Bytes const& program)
{
        Byte const opcode = program[pc];
        Instruction const instruction = translate_opcode(opcode);
        instruction(*this, program);
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

}

