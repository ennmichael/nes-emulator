#include "cpu.h"

namespace Emulator
{

namespace
{

void update_zero_flag(CPU& cpu, int result) noexcept
{
        cpu.status(CPU::zero_flag, result == 0);
}

void update_overflow_flag(CPU& cpu, int signed_result) noexcept
{
        cpu.status(CPU::overflow_flag,
                   signed_result < signed_byte_min ||
                   signed_result > signed_byte_max);
}

void update_negative_flag(CPU& cpu, unsigned result) noexcept
{
        cpu.status(CPU::negative_flag, Utils::sign_bit(result));
}

void update_negative_flag(CPU& cpu, int result) noexcept
{
        update_negative_flag(cpu, static_cast<unsigned>(result));
}

template <class Integer>
void update_transfer_flags(CPU& cpu, Integer i) noexcept
{
        update_zero_flag(cpu, i);
        update_negative_flag(cpu, i);
}

void execute_on_memory(Byte (*operation)(CPU&),
                       CPU& cpu,
                       unsigned address)
{
        cpu.memory->write_byte(address, operation(cpu));
}

void execute_on_memory(void (*operation)(CPU&, Byte),
                       CPU& cpu,
                       unsigned address)
{
        auto const operand = cpu.memory->read_byte(address);
        operation(cpu, operand);
}

void execute_on_memory(Byte (*operation)(CPU&, Byte),
                       CPU& cpu,
                       unsigned address)
{
        auto const operand = cpu.memory->read_byte(address);
        cpu.memory->write_byte(address, operation(cpu, operand));
}

template <class Operation, class Offset>
Instruction zero_page(Operation operation, Offset offset)
{
        return [operation, offset](CPU& cpu)
        {
                auto const base_address = cpu.memory->read_byte(cpu.pc + 1);
                unsigned const address =
                        base_address + static_cast<unsigned>(offset(cpu));
                execute_on_memory(operation, cpu, address);
                cpu.pc += 2;
        };
}

template <class Operation>
Instruction zero_page(Operation operation)
{
        return zero_page(operation, [](CPU const&) { return 0; });
}

template <class Operation>
Instruction zero_page_x(Operation operation)
{
        return zero_page(operation, [](CPU const& cpu) { return cpu.x; });
}

template <class Operation>
Instruction zero_page_y(Operation operation)
{
        return zero_page(operation, [](CPU const& cpu) { return cpu.y; });
}

template <class Operation, class Offset>
Instruction absolute(Operation operation, Offset offset)
{
        return [operation, offset](CPU& cpu)
        {
                unsigned const address =
                        cpu.memory->read_pointer(cpu.pc + 1) + offset(cpu);
                execute_on_memory(operation, cpu, address);
                cpu.pc += 3;
        };
}

template <class Operation>
Instruction absolute(Operation operation)
{
        return absolute(operation, [](CPU const&) { return 0; });
}

template <class Operation>
Instruction absolute_x(Operation operation)
{
        return absolute(operation, [](CPU const& cpu) { return cpu.x; });
}

template <class Operation>
Instruction absolute_y(Operation const& operation)
{
        return absolute(operation, [](CPU const& cpu) { return cpu.y; });
}

template <class Operation>
Instruction indirect(Operation operation)
{
        return [operation](CPU& cpu)
        {
                unsigned const address = cpu.memory->deref_pointer(cpu.pc);
                execute_on_memory(operation, cpu, address);
                cpu.pc += 3;
        };
}

template <class Operation>
Instruction implied(Operation operation)
{
        return [operation](CPU& cpu)
        {
                operation(cpu);
                cpu.pc += 1;
        };
}

template <class Operation>
Instruction accumulator(Operation operation)
{
        return [operation](CPU& cpu)
        {
                cpu.a = operation(cpu, cpu.a);
                cpu.pc += 1;
        };
}

template <class Operation>
Instruction immediate(Operation operation)
{
        return [operation](CPU& cpu)
        {
                Byte const operand = cpu.memory->read_pointer(cpu.pc + 1);
                operation(cpu, operand);
                cpu.pc += 2;
        };
}

template <class Branch>
Instruction relative(Branch branch)
{
        return [branch](CPU& cpu)
        {
                if (branch(cpu))
                        cpu.pc += static_cast<int>(cpu.memory->read_byte(cpu.pc + 1));
                cpu.pc += 2;
        };
}

template <class Operation>
Instruction indirect_x(Operation operation) // Indexed indirect
{
        return [operation](CPU& cpu)
        {
                unsigned const zero_page_address = cpu.memory->read_byte(cpu.pc + 1);
                unsigned const pointer =
                        cpu.memory->read_pointer(zero_page_address + cpu.x);
                execute_on_memory(operation, cpu, pointer);
                cpu.pc += 2;
        };
}

template <class Operation>
Instruction indirect_y(Operation operation) // Indirect indexed
{
        return [operation](CPU& cpu)
        {
                Byte const zero_page_address = cpu.memory->read_byte(cpu.pc + 1);
                unsigned const pointer =
                        cpu.memory->read_pointer(zero_page_address) + cpu.y;
                execute_on_memory(operation, cpu, pointer);
                cpu.pc += 2;
        };
}

bool bcs(CPU const& cpu) noexcept
{
        return cpu.status(CPU::carry_flag);
}

bool bcc(CPU const& cpu) noexcept
{
        return !bcs(cpu);
}

bool beq(CPU const& cpu) noexcept
{
        return cpu.status(CPU::zero_flag);
}

bool bne(CPU const& cpu) noexcept
{
        return !beq(cpu);
}

bool bmi(CPU const& cpu) noexcept
{
        return cpu.status(CPU::negative_flag);
}

bool bpl(CPU const& cpu) noexcept
{
        return !bmi(cpu);
}

bool bvs(CPU const& cpu) noexcept
{
        return cpu.status(CPU::overflow_flag);
}

bool bvc(CPU const& cpu) noexcept
{
        return !bvs(cpu);
}

void transfer(CPU& cpu, Byte& reg, Byte value) noexcept
{
        reg = value;
        update_transfer_flags(cpu, reg);
}

void adc(CPU& cpu, Byte operand) noexcept
{
        int const signed_result = Utils::twos_complement(cpu.a) +
                                  Utils::twos_complement(operand) +
                                  cpu.status(CPU::carry_flag);
        auto const result = static_cast<Byte>(signed_result);

        cpu.status(CPU::carry_flag, cpu.a > result);
        update_overflow_flag(cpu, signed_result);

        transfer(cpu, cpu.a, result);
}

void sbc(CPU& cpu, Byte operand) noexcept
{
        /**
         * I believe this implementation gives the right results,
         * however it doesn't set the carry bit right. I should figure out:
         * 1) Why does it give correct results, if it does, and
         * 2) How I can fix the carry flag issue.
         */
        int const signed_result = Utils::twos_complement(cpu.a) -
                                  Utils::twos_complement(operand) -
                                  !cpu.status(CPU::carry_flag);

        cpu.status(CPU::carry_flag, signed_result <= byte_max);
        update_overflow_flag(cpu, signed_result);

        transfer(cpu, cpu.a, static_cast<Byte>(signed_result));
}

void bitwise_and(CPU& cpu, Byte operand) noexcept
{
        transfer(cpu, cpu.a, cpu.a & operand);
}

Byte asl(CPU& cpu, Byte operand) noexcept
{
        Byte const result = operand << 1;
        
        cpu.status(CPU::carry_flag, Utils::sign_bit(operand));
        update_transfer_flags(cpu, result);

        return result;
}

void bit(CPU& cpu, Byte operand) noexcept
{
        Byte const result = cpu.a & operand;
        ByteBitset const operand_bits = operand;

        cpu.status(CPU::negative_flag, operand_bits.test(CPU::negative_flag));
        cpu.status(CPU::overflow_flag, operand_bits.test(CPU::overflow_flag));
        update_zero_flag(cpu, result);
}

void brk(CPU& cpu) noexcept
{
        cpu.status(CPU::break_flag, true);
        cpu.pc += 1;
        cpu.raise_interrupt(CPU::Interrupt::irq);
}

void clc(CPU& cpu) noexcept
{
        cpu.status(CPU::carry_flag, false);
}

void cli(CPU& cpu) noexcept
{
        cpu.status(CPU::interrupt_disable_flag, false);
}

void clv(CPU& cpu) noexcept
{
        cpu.status(CPU::overflow_flag, false);
}

void compare(CPU& cpu, Byte reg, Byte operand) noexcept
{
        int const result = reg - operand;
        cpu.status(CPU::carry_flag, result >= 0);
        update_transfer_flags(cpu, result);
}

void cmp(CPU& cpu, Byte operand) noexcept
{
        compare(cpu, cpu.a, operand);
}

void cpx(CPU& cpu, Byte operand) noexcept
{
        compare(cpu, cpu.x, operand);
}

void cpy(CPU& cpu, Byte operand) noexcept
{
        compare(cpu, cpu.y, operand);
}

Byte dec(CPU& cpu, Byte operand) noexcept
{
        Byte const result = operand - 1;
        update_transfer_flags(cpu, result);
        return result;
}

void dex(CPU& cpu) noexcept
{
        cpu.x = dec(cpu, cpu.x);
}

void dey(CPU& cpu) noexcept
{
        cpu.y = dec(cpu, cpu.y);
}

void eor(CPU& cpu, Byte operand) noexcept
{
        transfer(cpu, cpu.a, cpu.a ^ operand);
}

Byte inc(CPU& cpu, Byte operand) noexcept
{
        Byte const result = operand + 1;
        update_transfer_flags(cpu, result);
        return result;
}

void inx(CPU& cpu) noexcept
{
        cpu.x = inc(cpu, cpu.x);
}

void iny(CPU& cpu) noexcept
{
        cpu.y = inc(cpu, cpu.y);
}

void absolute_jmp(CPU& cpu) noexcept
{
        cpu.pc = cpu.memory->read_pointer(cpu.pc + 1);
}

void indirect_jmp(CPU& cpu) noexcept
{
        cpu.pc = cpu.memory->deref_pointer(cpu.pc + 1);
}

void absolute_jsr(CPU& cpu) noexcept
{
        Stack::push_pointer(cpu, cpu.pc);
        cpu.pc = cpu.memory->read_pointer(cpu.pc);
}

void lda(CPU& cpu, Byte operand) noexcept
{
        transfer(cpu, cpu.a, operand);
}

void ldx(CPU& cpu, Byte operand) noexcept
{
        transfer(cpu, cpu.x, operand);
}

void ldy(CPU& cpu, Byte operand) noexcept
{
        transfer(cpu, cpu.y, operand);
}

Byte lsr(CPU& cpu, Byte operand) noexcept
{
        Byte const result = operand >> 1;

        cpu.status(CPU::carry_flag, Utils::zeroth_bit(operand));
        update_transfer_flags(cpu, result);

        return result;
}

void nop(CPU&) noexcept
{}

void ora(CPU& cpu, Byte operand) noexcept
{
        transfer(cpu, cpu.a, cpu.a | operand);
}

void pha(CPU& cpu) noexcept
{
        Stack::push_byte(cpu, cpu.a);
}

void php(CPU& cpu) noexcept
{
        Stack::push_byte(cpu, Utils::to_byte(cpu.p));
}

void pla(CPU& cpu) noexcept
{
        transfer(cpu, cpu.a, Stack::pull_byte(cpu));
}

void plp(CPU& cpu) noexcept
{
        cpu.p = Stack::pull_byte(cpu);
        cpu.status(CPU::unused_flag, true);
}

Byte rol(CPU& cpu, Byte operand) noexcept
{
        Byte const result = Utils::set_zeroth_bit(operand << 1,
                                                  cpu.status(CPU::carry_flag));

        cpu.status(CPU::carry_flag, Utils::sign_bit(operand));
        update_transfer_flags(cpu, result);

        return result;
}

Byte ror(CPU& cpu, Byte operand) noexcept
{
        Byte const result = Utils::set_sign_bit(operand >> 1,
                                                cpu.status(CPU::carry_flag));

        cpu.status(CPU::carry_flag, Utils::zeroth_bit(operand));
        update_transfer_flags(cpu, result);

        return result;
}

void rti(CPU& cpu) noexcept
{
        cpu.p = Stack::pull_byte(cpu);
        cpu.pc = Stack::pull_pointer(cpu);
}

void rts(CPU& cpu) noexcept
{
        cpu.pc = Stack::pull_pointer(cpu);
}

void sec(CPU& cpu) noexcept
{
        cpu.status(CPU::carry_flag, true);
}

void sei(CPU& cpu) noexcept
{
        cpu.status(CPU::interrupt_disable_flag, true);
}

Byte sta(CPU& cpu) noexcept
{
        return cpu.a;
}

Byte stx(CPU& cpu) noexcept
{
        return cpu.x;
}

Byte sty(CPU& cpu) noexcept
{
        return cpu.y;
}

void tax(CPU& cpu) noexcept
{
        transfer(cpu, cpu.x, cpu.a);
}

void tay(CPU& cpu) noexcept
{
        transfer(cpu, cpu.y, cpu.a);
}

void tsx(CPU& cpu) noexcept
{
        transfer(cpu, cpu.x, cpu.sp);
}

void txa(CPU& cpu) noexcept
{
        transfer(cpu, cpu.a, cpu.x);
}

void txs(CPU& cpu) noexcept
{
        transfer(cpu, cpu.sp, cpu.x);
}

void tya(CPU& cpu) noexcept
{
        transfer(cpu, cpu.a, cpu.y);
}

}

Instruction CPU::translate_opcode(Byte opcode)
{
        switch (opcode) {
                case 0x00: return implied(brk);
                case 0x01: return indirect_x(ora);
                case 0x05: return zero_page(ora);
                case 0x06: return zero_page(asl);
                case 0x08: return implied(php);
                case 0x09: return immediate(ora);
                case 0x0A: return accumulator(asl);
                case 0x0D: return absolute(ora);
                case 0x0E: return absolute(asl);
                case 0x10: return relative(bpl);
                case 0x11: return indirect_y(ora);
                case 0x15: return zero_page_x(ora);
                case 0x16: return zero_page_x(asl);
                case 0x18: return implied(clc);
                case 0x19: return absolute_y(ora);
                case 0x1D: return absolute_x(ora);
                case 0x1E: return absolute_x(asl);
                case 0x20: return absolute_jsr;
                case 0x21: return indirect_x(bitwise_and);
                case 0x24: return zero_page(bit);
                case 0x25: return zero_page(bitwise_and);
                case 0x26: return zero_page(rol);
                case 0x28: return implied(plp);
                case 0x29: return immediate(bitwise_and);
                case 0x2A: return accumulator(rol);
                case 0x2C: return absolute(bit);
                case 0x2D: return absolute(bitwise_and);
                case 0x2E: return absolute(rol);
                case 0x30: return relative(bmi);
                case 0x31: return indirect_y(bitwise_and);
                case 0x35: return zero_page_x(bitwise_and);
                case 0x36: return zero_page_x(rol);
                case 0x38: return implied(sec);
                case 0x39: return absolute_y(bitwise_and);
                case 0x3D: return absolute_x(bitwise_and);
                case 0x3E: return absolute_x(rol);
                case 0x40: return implied(rti);
                case 0x41: return indirect_x(eor);
                case 0x45: return zero_page(eor);
                case 0x46: return zero_page(lsr);
                case 0x48: return implied(pha);
                case 0x49: return immediate(eor);
                case 0x4A: return accumulator(lsr);
                case 0x4C: return absolute_jmp;
                case 0x4D: return absolute(eor);
                case 0x4E: return absolute(lsr);
                case 0x50: return relative(bvc);
                case 0x51: return indirect_y(eor);
                case 0x55: return zero_page_x(eor);
                case 0x56: return zero_page_x(lsr);
                case 0x58: return implied(cli);
                case 0x59: return absolute_y(eor);
                case 0x5D: return absolute_x(eor);
                case 0x5E: return absolute_x(lsr);
                case 0x60: return implied(rts);
                case 0x61: return indirect_x(adc);
                case 0x65: return zero_page(adc);
                case 0x66: return zero_page(ror);
                case 0x68: return implied(pla);
                case 0x69: return immediate(adc);
                case 0x6A: return accumulator(ror);
                case 0x6C: return indirect_jmp;
                case 0x6D: return absolute(adc);
                case 0x6E: return absolute(ror);
                case 0x70: return relative(bvs);
                case 0x71: return indirect_y(adc);
                case 0x75: return zero_page_x(adc);
                case 0x76: return zero_page_x(ror);
                case 0x78: return implied(sei);
                case 0x79: return absolute_y(adc);
                case 0x7D: return absolute_x(adc);
                case 0x7E: return absolute_x(ror);
                case 0x81: return indirect_x(sta);
                case 0x84: return zero_page(sty);
                case 0x85: return zero_page(sta);
                case 0x86: return zero_page(stx);
                case 0x88: return implied(dey);
                case 0x8A: return implied(txa);
                case 0x8C: return absolute(sty);
                case 0x8D: return absolute(sta);
                case 0x8E: return absolute(stx);
                case 0x90: return relative(bcc);
                case 0x91: return indirect_y(sta);
                case 0x94: return zero_page_x(sty);
                case 0x95: return zero_page_x(sta);
                case 0x96: return zero_page_y(stx);
                case 0x98: return implied(tya);
                case 0x99: return absolute_y(sta);
                case 0x9A: return implied(txs);
                case 0x9D: return absolute_x(sta);
                case 0xA0: return immediate(ldy);
                case 0xA1: return indirect_x(lda);
                case 0xA2: return immediate(ldx);
                case 0xA4: return zero_page(ldy);
                case 0xA5: return zero_page(lda);
                case 0xA6: return zero_page(ldx);
                case 0xA8: return implied(tay);
                case 0xA9: return immediate(lda);
                case 0xAA: return implied(tax);
                case 0xAC: return absolute(ldy);
                case 0xAD: return absolute(lda);
                case 0xAE: return absolute(ldx);
                case 0xB0: return relative(bcs);
                case 0xB1: return indirect_y(lda);
                case 0xB4: return zero_page_x(ldy);
                case 0xB5: return zero_page_x(lda);
                case 0xB6: return zero_page_y(ldx);
                case 0xB8: return implied(clv);
                case 0xB9: return absolute_y(lda);
                case 0xBA: return implied(tsx);
                case 0xBC: return absolute_x(ldy);
                case 0xBD: return absolute_x(lda);
                case 0xBE: return absolute_y(ldx);
                case 0xC0: return immediate(cpy);
                case 0xC1: return indirect_x(cmp);
                case 0xC4: return zero_page(cpy);
                case 0xC5: return zero_page(cmp);
                case 0xC6: return zero_page(dec);
                case 0xC8: return implied(iny);
                case 0xC9: return immediate(cmp);
                case 0xCA: return implied(dex);
                case 0xCC: return absolute(cpy);
                case 0xCD: return absolute(cmp);
                case 0xCE: return absolute(dec);
                case 0xD0: return relative(bne);
                case 0xD1: return indirect_y(cmp);
                case 0xD5: return zero_page_x(cmp);
                case 0xD6: return zero_page_x(dec);
                case 0xD9: return absolute_y(cmp);
                case 0xDD: return absolute_x(cmp);
                case 0xDE: return absolute_x(dec);
                case 0xE0: return immediate(cpx);
                case 0xE1: return indirect_x(sbc);
                case 0xE4: return zero_page(cpx);
                case 0xE5: return zero_page(sbc);
                case 0xE6: return zero_page(inc);
                case 0xE8: return implied(inx);
                case 0xE9: return immediate(sbc);
                case 0xEA: return implied(nop);
                case 0xEC: return absolute(cpx);
                case 0xED: return absolute(sbc);
                case 0xEE: return absolute(inc);
                case 0xF0: return relative(beq);
                case 0xF1: return indirect_y(sbc);
                case 0xF5: return zero_page_x(sbc);
                case 0xF6: return zero_page_x(inc);
                case 0xF9: return absolute_y(sbc);
                case 0xFD: return absolute_x(sbc);
                case 0xFE: return absolute_x(inc);
                default: throw UnknownOpcode(opcode);
        }
}

}

