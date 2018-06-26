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

void CPU::Memory::write(unsigned address, Byte byte) noexcept
{
        ram_[address % internal_ram_size] = byte;
}

Byte CPU::Memory::read(unsigned address) const noexcept
{
        return ram_[address % internal_ram_size];
}

unsigned CPU::Memory::read_address(unsigned pointer) const noexcept
{
        Byte const low = read(pointer);
        Byte const high = read(pointer + 1);
        return Utils::combine_little_endian(low, high);
}

CPU::CPU(Cartridge& cartridge, PPU& ppu) noexcept
        : memory(cartridge, ppu)
{}

void CPU::execute_program(Bytes const& program)
{
        while (pc != program.size())
                execute_instruction(program);
}

void CPU::execute_instruction(Bytes const& program)
{
        Byte const opcode = program[pc];
        Instruction const instruction = translate_opcode(opcode);
        instruction(*this, program);
}

bool CPU::status(unsigned flag) const noexcept
{
        return p.test(flag);
}

Byte CPU::read(unsigned address) const noexcept
{
        return memory_.read(address);
}

unsigned CPU::read_address(unsigned pointer) const noexcept
{
        return memory_.read_address(pointer);
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
                case 0x20: return absolute(jsr);
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
                case 0xD8: return implied(cld);
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
                case 0xF8: return implied(sed);
                case 0xF9: return absolute_y(sbc);
                case 0xFD: return absolute_x(sbc);
                case 0xFE: return absolute_x(inc);
                default: throw UnknownOpcode(opcode);
        }
}

unsigned CPU::fetch_absolute_address(Bytes const& program) const noexcept
{
        return Utils::combine_little_endian(program[pc_ + 1], program[pc_ + 2]);
}

unsigned CPU::fetch_indirect_address(Bytes const& program) const noexcept
{
        unsigned const pointer = fetch_absolute_address(program);
        return memory_.read_address(pointer);
}

bool CPU::bcs(CPU const& cpu) noexcept
{
        return cpu.status(carry_flag);
}

bool CPU::bcc(CPU const& cpu) noexcept
{
        return !bcs(cpu);
}

bool CPU::beq(CPU const& cpu) noexcept
{
        return cpu.status(zero_flag);
}

bool CPU::bne(CPU const& cpu) noexcept
{
        return !beq(cpu);
}

bool CPU::bmi(CPU const& cpu) noexcept
{
        return cpu.status(negative_flag);
}

bool CPU::bpl(CPU const& cpu) noexcept
{
        return !bmi(cpu);
}

bool CPU::bvs(CPU const& cpu) noexcept
{
        return cpu.status(overflow_flag);
}

bool CPU::bvc(CPU const& cpu) noexcept
{
        return !bvs(cpu);
}

void CPU::adc(CPU& cpu, Byte operand) noexcept
{
        unsigned const result = cpu.a_ + operand + cpu.status(carry_flag);

        cpu.status(carry_flag, result > 255);
        cpu.update_overflow_flag(unsigned_result);
        cpu.update_negative_flag(unsigned_result);
        cpu.update_zero_flag(signed_result);

        cpu.a_ = static_cast<Byte>(result);
}

void CPU::sbc(CPU& cpu, Byte operand) noexcept
{
        int const signed_result = cpu.a_ - operand - !status(carry_flag);
        auto const unsigned_result = reinterpret_cast<unsigned>(signed_result);

        cpu.status(carry_flag, signed_result >= 0);
        cpu.update_overflow_flag(unsigned_result);
        cpu.update_negative_flag(unsigned_result);
        cpu.update_zero_flag(signed_result);

        cpu.a_ = static_cast<Byte>(unsigned_result);
}

void CPU::bitwise_and(CPU& cpu, Byte operand) noexcept
{
        cpu.a_ &= operand;
        
        cpu.update_zero_flag(cpu.a_);
        cpu.update_negative_flag(cpu.a_);
}

Byte CPU::asl(CPU& cpu, Byte operand) noexcept
{
        Byte const result = operand << 1;
        
        cpu.status(carry_flag, Utils::sign_bit(operand));
        cpu.update_zero_flag(result);
        cpu.update_negative_flag(result);

        return result;
}

void CPU::bit(CPU& cpu, Byte operand) noexcept
{
        Byte const result = cpu.a_ & operand;

        cpu.status(overflow_flag, result & 0x40);
        cpu.update_zero_flag(result);
        cpu.update_negative_flag(result);
}

void CPU::brk(CPU& cpu, Byte operand) noexcept
{
        // FIXME cause the actual interrupt
        cpu.status(break_flag, true);
}

void CPU::clc(CPU& cpu, Byte operand) noexcept
{
        cpu.status(carry_flag, false);
}

void CPU::cli(CPU& cpu, Byte operand) noexcept
{
        cpu.status(interrupt_disable_flag, false);
}

void CPU::clv(CPU& cpu, Byte operand) noexcept
{
        cpu.status(overflow_flag, false);
}

void CPU::cmp(CPU& cpu, Byte operand) noexcept
{
        compare(cpu, cpu.a_, operand);
}

void CPU::cpx(CPU& cpu, Byte operand) noexcept
{
        compare(cpu, cpu.x_, operand);
}

void CPU::cpy(CPU& cpu, Byte operand) noexcept
{
        compare(cpu, cpu.y_, operand);
}

Byte CPU::dec(CPU& cpu, Byte operand) noexcept
{
        Byte const result = operand - 1;

        cpu.update_zero_flag(result);
        cpu.update_negative_flag(result);

        return result;
}

void CPU::dex(CPU& cpu, Byte operand) noexcept
{
        cpu.x_ = dec(cpu, cpu.x_);
}

void CPU::dey(CPU& cpu, Byte operand) noexcept
{
        cpu.y_ = dec(cpu, cpu.y_);
}

void CPU::eor(CPU& cpu, Byte operand) noexcept
{
        cpu.a_ ^= operand;

        cpu.update_zero_flag(cpu.a_);
        cpu.update_negative_flag(cpu.a_);
}

Byte CPU::inc(CPU& cpu, Byte operand) noexcept
{
        Byte const result = operand + 1;

        cpu.update_zero_flag(result);
        cpu.update_negative_flag(result);

        return result;
}

void CPU::inx(CPU& cpu, Byte operand) noexcept
{
        cpu.x_ = inc(cpu, cpu.x_);
}

void CPU::iny(CPU& cpu, Byte operand) noexcept
{
        cpu.y_ = inc(cpu, cpu.y_);
}

void CPU::absolute_jmp(CPU& cpu, Bytes const& program) noexcept
{
        cpu.pc_ = cpu.fetch_absolute_address(program);
}

void CPU::indirect_jmp(CPU& cpu, Bytes const& program) noexcept
{
        cpu.pc_ = cpu.fetch_indirect_address(program);
}

void CPU::jsr(CPU& cpu, Byte operand) noexcept
{
        // TODO Shit to do with the stack
}

void CPU::lda(CPU& cpu, Byte operand) noexcept
{
        cpu.a_ = operand;

        cpu.update_zero_flag(operand);
        cpu.update_negative_flag(operand);
}

void CPU::ldx(CPU& cpu, Byte operand) noexcept
{
        cpu.x_ = operand;

        cpu.update_zero_flag(operand);
        cpu.update_negative_flag(operand);
}

void CPU::ldy(CPU& cpu, Byte operand) noexcept
{
        cpu.y_ = operand;

        cpu.update_zero_flag(operand);
        cpu.update_negative_flag(operand);
}

Byte CPU::lsr(CPU& cpu, Byte operand) noexcept
{
        Byte const result = operand >> 1;

        cpu.status(carry_flag, Utils::zeroth_bit(result));
        cpu.update_zero_flag(result);
        cpu.update_negative_flag(result);

        return result;
}

void CPU::nop(CPU& cpu, Byte operand) noexcept
{}

void CPU::ora(CPU& cpu, Byte operand) noexcept
{
        cpu.a_ |= operand;

        cpu.update_zero_flag(cpu.a_);
        cpu.update_negative_flag(cpu.a_);
}

void CPU::pha(CPU& cpu, Byte operand) noexcept
{
        // TODO
}

void CPU::php(CPU& cpu, Byte operand) noexcept
{
        // TODO
}

void CPU::pla(CPU& cpu, Byte operand) noexcept
{
        // TODO
}

void CPU::plp(CPU& cpu, Byte operand) noexcept
{
        // TODO
}

Byte CPU::rol(CPU& cpu, Byte operand) noexcept
{
        Byte const result = Utils::set_zeroth_bit(operand << 1,
                                                  cpu.status(carry_flag));

        cpu.status(carry_flag, Utils::sign_bit(operand));
        cpu.update_zero_flag(result);
        cpu.update_negative_flag(result);

        return result;
}

void CPU::ror(CPU& cpu, Byte operand) noexcept
{
        Byte const result = Utils::set_sign_bit(operand >> 1,
                                                cpu.status(carry_flag));

        cpu.status(carry_flag, Utils::zeroth_bit(operand));
        cpu.update_zero_flag(result);
        cpu.update_negative_flag(result);

        return result;
}

void CPU::rti(CPU& cpu, Byte operand) noexcept
{
        // TODO
}

void CPU::rts(CPU& cpu, Byte operand) noexcept
{
        // TODO
}

void CPU::sec(CPU& cpu, Byte operand) noexcept
{
        cpu.status(carry_flag, true);
}

void CPU::sei(CPU& cpu, Byte operand) noexcept
{
        cpu.status(interrupt_disable_flag, true);
}

Byte CPU::sta(CPU& cpu) noexcept
{
        return cpu.a_;
}

Byte CPU::stx(CPU& cpu) noexcept
{
        return cpu.x_;
}

Byte CPU::sty(CPU& cpu) noexcept
{
        return cpu.y_;
}

void CPU::tax(CPU& cpu) noexcept
{
        cpu.x_ = cpu.a_;

        cpu.update_zero_flag(cpu.x_);
        cpu.update_negative_flag(cpu.x_);
}

void CPU::tay(CPU& cpu, Byte operand) noexcept
{
        cpu.y_ = cpu.a_;

        cpu.update_zero_flag(cpu.y_);
        cpu.update_negative_flag(cpu.y_);
}

void CPU::tsx(CPU& cpu, Byte operand) noexcept
{
        // TODO
}

void CPU::txa(CPU& cpu, Byte operand) noexcept
{
        cpu.a_ = cpu.x_;

        cpu.update_zero_flag(cpu.a_);
        cpu.update_negative_flag(cpu.a_);
}

void CPU::txs(CPU& cpu, Byte operand) noexcept
{
        // TODO
}

void CPU::tya(CPU& cpu, Byte operand) noexcept
{
        cpu.a_ = cpu.y_;

        cpu.update_zero_flag(cpu.a_);
        cpu.update_negative_flag(cpu.a_);
}

void CPU::compare(CPU& cpu, Byte reg, Byte operand) noexcept
{
        int const result = reg - operand;

        cpu.status(carry_flag, result >= 0);
        cpu.update_zero_flag(result);
        cpu.update_negative_flag(reinterpret_cast<unsigned>(result));
}

void CPU::update_overflow_flag(unsigned old_value, unsigned result) noexcept
{
        status(overflow_flag, Utils::sign_bit(old_value) != Utils::sign_bit(result));
}

void CPU::update_negative_flag(unsigned result) noexcept
{
        status(negative_flag,  Utils::sign_bit(old_value));
}

void CPU::write(unsigned address, Byte byte) noexcept
{
        memory_.write(address, byte);
}

void CPU::status(unsigned flag, bool value) noexcept
{
        p.set(flag, value);
}

}

