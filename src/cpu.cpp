#include "cpu.h"
#include <utility>
#include <string>
#include <sstream>
#include <cassert>

// CPU::translate_opcode is defined in instruction_set.cpp

using namespace std::string_literals;

namespace Emulator {

UnknownOpcode::UnknownOpcode(Byte opcode) noexcept
        : runtime_error("Unknown opcode "s + Utils::format_hex(opcode, 2) + "."s)
{}

bool CPU::RAM::address_is_accessible(unsigned address) noexcept
{
        return start <= address && address < end;
}

bool CPU::RAM::address_is_writable(unsigned address) const noexcept
{
        return address_is_accessible(address);
}

bool CPU::RAM::address_is_readable(unsigned address) const noexcept
{
        return address_is_accessible(address);
}

void CPU::RAM::write_byte(unsigned address, Byte byte)
{
        if (!address_is_accessible(address)) {
                throw InvalidAddress("Can't write to CPU RAM at address "s +
                                     Utils::format_address(address) +
                                     ". Valid range is "s +
                                     Utils::format_address(start) +
                                     " to "s +
                                     Utils::format_address(end) +
                                     "."s);
        }

        ram_[translate_address(address)] = byte;
}

Byte CPU::RAM::read_byte(unsigned address)
{
        RAM const& self = *this;
        return self.read_byte(address);
}

Byte CPU::RAM::read_byte(unsigned address) const
{
        if (!address_is_accessible(address)) {
                throw InvalidAddress("Can't read CPU RAM at address "s +
                                     Utils::format_address(address) +
                                     ". Valid range is "s +
                                     Utils::format_address(start) +
                                     " to "s +
                                     Utils::format_address(end) +
                                     "."s);
        }

        return ram_[translate_address(address)];
}

unsigned CPU::RAM::translate_address(unsigned address) const noexcept
{
        return address % real_size;
}

struct CPU::Impl {
        struct AccessibleMemory : Memory {
                void write_byte(unsigned address, Byte byte)
                {
                        if (ram.address_is_writable(address)) {
                                ram.write_byte(address, byte);
                        } else if (cartridge.address_is_writable(address)) {
                                cartridge.write_byte(address, byte);
                        } else if (ppu.address_is_writable(address)) {
                                ppu.write_byte(address, byte);
                        } else {
                                throw InvalidAddress("CPU can't write to address "s +
                                                     Utils::format_address(address));
                        }
                }

                Byte read_byte(unsigned address)
                {
                        if (ram.address_is_readable(address))
                                return ram.read_byte(address);
                        if (cartridge.address_is_readable(address))
                                return cartridge.read_byte(address);
                        if (ppu.address_is_readable(address))
                                return ppu.read_byte(address);
                        throw InvalidAddress("CPU can't read address "s +
                                             Utils::format_address(address));
                }

                RAM ram;
                Memory& cartridge;
                Memory& ppu;
        };

        explicit Impl(Memory& cartridge, Memory& ppu) noexcept
                : memory {
                        .cartridge = cartridge,
                        .ppu = ppu
                }
        {
                load_interrupt_handler(Interrupt::reset);
        }

        unsigned stack_top_address() const noexcept
        {
                return stack_bottom_address + sp;
        }

        Byte stack_top_byte() const
        {
                return memory.ram.read_byte(stack_top_address() + 1);
        }

        unsigned stack_top_pointer() const
        {
                return memory.ram.read_pointer(stack_top_address() + 1);
        }

        void stack_push_byte(Byte byte)
        {
                memory.ram.write_byte(stack_top_address(), byte);
                sp -= 1;
        }

        void stack_push_pointer(unsigned pointer)
        {
                memory.ram.write_pointer(stack_top_address() - 1, pointer);
                sp -= address_size;
        }

        Byte stack_pull_byte(CPU& cpu)
        {
                Byte const result = stack_top_byte();
                sp += 1;
                return result;
        }

        unsigned CPU::interrupt_handler(Interrupt interrupt) const noexcept
        {
                unsigned const pointer_address = interrupt_handler_address(interrupt);
                return memory->read_pointer(pointer_address);
        }

        void CPU::load_interrupt_handler(Interrupt interrupt) noexcept
        {
                pc = interrupt_handler(interrupt);
        }

        static unsigned interrupt_handler_address(Interrupt interrupt) noexcept
        {
                switch (interrupt) {
                        case Interrupt::nmi:   return 0xFFFA;
                        case Interrupt::reset: return 0xFFFC;
                        case Interrupt::irq:   return 0xFFFE;
                }

                assert(false);
                return 0; 
        }

        template <class Integer>
        void update_transfer_flags(Integer i) noexcept
        {
                update_zero_flag(i);
                update_negative_flag(i);
        }

        template <class Operation, class Offset>
        Instruction zero_page(Operation operation, Offset offset)
        {
                return [this, operation, offset]
                {
                        auto const base_address = memory->read_byte(pc + 1);
                        unsigned const address = base_address + offset();
                        execute_on_memory(operation, address);
                        pc += 2;
                };
        }

        template <class Operation>
        Instruction zero_page(Operation operation)
        {
                return zero_page(operation, [] { return 0; });
        }

        template <class Operation>
        Instruction zero_page_x(Operation operation)
        {
                return zero_page(operation, [this] { return x; });
        }

        template <class Operation>
        Instruction zero_page_y(Operation operation)
        {
                return zero_page(operation, [this] { return y; });
        }

        template <class Operation, class Offset>
        Instruction absolute(Operation operation, Offset offset)
        {
                return [this, operation, offset]
                {
                        unsigned const address =
                                memory->read_pointer(pc + 1) + offset();
                        execute_on_memory(operation, address);
                        pc += 3;
                };
        }

        template <class Operation>
        Instruction absolute(Operation operation)
        {
                return absolute(operation, [] { return 0; });
        }

        template <class Operation>
        Instruction absolute_x(Operation operation)
        {
                return absolute(operation, [this] { return x; });
        }

        template <class Operation>
        Instruction absolute_y(Operation const& operation)
        {
                return absolute(operation, [this] { return y; });
        }

        template <class Operation>
        Instruction indirect(Operation operation)
        {
                return [this, operation]
                {
                        unsigned const address = memory->deref_pointer(pc);
                        execute_on_memory(operation, address);
                        pc += 3;
                };
        }

        template <class Operation>
        Instruction implied(Operation operation)
        {
                return [this, operation]
                {
                        operation(*this);
                        pc += 1;
                };
        }

        template <class Operation>
        Instruction accumulator(Operation operation)
        {
                return [this, operation]
                {
                        a = operation(*this, a);
                        pc += 1;
                };
        }

        template <class Operation>
        Instruction immediate(Operation operation)
        {
                return [this, operation]
                {
                        auto const operand = memory->read_byte(pc + 1);
                        operation(*this, operand);
                        pc += 2;
                };
        }

        template <class Branch>
        Instruction relative(Branch branch)
        {
                return [this, branch]
                {
                        if (branch(*this)) {
                                auto const displacement = memory->read_byte(pc + 1);
                                pc += TwosComplement::encode(displacement);
                        }
                        pc += 2;
                };
        }

        template <class Operation>
        Instruction indirect_x(Operation operation) // Indexed indirect
        {
                return [this, operation]
                {
                        unsigned const zero_page_address = memory->read_byte(pc + 1);
                        unsigned const pointer =
                                memory->read_pointer(zero_page_address + x);
                        execute_on_memory(operation, pointer);
                        pc += 2;
                };
        }

        template <class Operation>
        static Instruction indirect_y(Operation operation) // Indirect indexed
        {
                return [this, operation]
                {
                        auto const zero_page_address = memory->read_byte(pc + 1);
                        unsigned const pointer =
                                memory->read_pointer(zero_page_address) + y;
                        execute_on_memory(operation, pointer);
                        pc += 2;
                };
        }

        void execute_on_memory(std::function<Byte(Impl& impl)> operation,
                               unsigned address)
        {
                memory->write_byte(address, operation(*this));
        }

        void execute_on_memory(std::function<void(Impl& impl, Byte operand)> opertion,
                               unsigned address)
        {
                auto const operand = memory->read_byte(address);
                operation(*this, operand);
        }

        void execute_on_memory(std::function<Byte(Impl& impl, Byte operand)> operation,
                               unsigned address)
        {
                auto const operand = memory->read_byte(address);
                memory->write_byte(address, operation(*this, operand));
        }

        void update_zero_flag(int result) noexcept
        {
                p.set(zero_flag, result == 0);
        }

        void update_overflow_flag(int signed_result) noexcept
        {
                p.set(overflow_flag,
                      signed_result < signed_byte_min ||
                      signed_result > signed_byte_max);
        }

        void update_negative_flag(unsigned result) noexcept
        {
                p.set(negative_flag, Utils::bit(result, sign_bit));
        } 

        bool bcs() const noexcept
        {
                return p.test(carry_flag);
        }

        bool bcc() const noexcept
        {
                return !bcs();
        }

        bool beq() const noexcept
        {
                return p.test(zero_flag);
        }

        bool bne() const noexcept
        {
                return !beq();
        }

        bool bmi() const noexcept
        {
                return p.test(negative_flag);
        }

        bool bpl() const noexcept
        {
                return !bmi();
        }

        bool bvs() const noexcept
        {
                return p.test(overflow_flag);
        }

        bool bvc() const noexcept
        {
                return !bvs();
        }

        void transfer(Byte& reg, Byte value) noexcept
        {
                reg = value;
                update_transfer_flags(reg);
        }

        void adc(Byte operand) noexcept
        {
                int const signed_result = TwosComplement::encode(a) +
                                          TwosComplement::encode(operand) +
                                          p.test(carry_flag);
                auto const result = TwosComplement::decode(signed_result);
                p.set(carry_flag, a > result);
                update_overflow_flag(signed_result);
                transfer(a, result);
        }

        void sbc(Byte operand) noexcept
        {
                int const signed_result = TwosComplement::encode(a) -
                                          TwosComplement::encode(operand) -
                                          !p.test(carry_flag);
                Byte const result = TwosComplement::decode(signed_result);
                p.set(carry_flag, operand <= a);
                update_overflow_flag(signed_result);
                transfer(a, result);
        }

        void bitwise_and(Byte operand) noexcept
        {
                transfer(a, a & operand);
        }

        Byte asl(Byte operand) noexcept
        {
                Byte const result = operand << 1;
                p.set(carry_flag, Utils::bit(operand, sign_bit));
                update_transfer_flags(result);
                return result;
        }

        void bit(Byte operand) noexcept
        {
                ByteBitset const operand_bits = operand;
                p.set(negative_flag, operand_bits.test(negative_flag));
                p.set(overflow_flag, operand_bits.test(overflow_flag));
                update_zero_flag(a & operand);
        }

        void clc() noexcept
        {
                p.set(carry_flag, false);
        }

        void cli() noexcept
        {
                p.set(interrupt_disable_flag, false);
        }

        void clv() noexcept
        {
                p.set(overflow_flag, false);
        }

        void compare(Byte reg, Byte operand) noexcept
        {
                int const result = reg - operand;
                p.set(carry_flag, result >= 0);
                update_transfer_flags(result);
        }

        void cmp(Byte operand) noexcept
        {
                compare(a, operand);
        }

        void cpx(Byte operand) noexcept
        {
                compare(x, operand);
        }

        void cpy(Byte operand) noexcept
        {
                compare(y, operand);
        }

        Byte dec(Byte operand) noexcept
        {
                Byte const result = operand - 1;
                update_transfer_flags(result);
                return result;
        }

        void dex() noexcept
        {
                x = dec(x);
        }

        void dey() noexcept
        {
                y = dec(y);
        }

        void eor(Byte operand) noexcept
        {
                transfer(a, a ^ operand);
        }

        Byte inc(Byte operand) noexcept
        {
                Byte const result = operand + 1;
                update_transfer_flags(result);
                return result;
        }

        void inx() noexcept
        {
                x = inc(x);
        }

        void iny() noexcept
        {
                y = inc(y);
        }

        void absolute_jmp() noexcept
        {
                pc = memory->read_pointer(pc + 1);
        }

        void indirect_jmp() noexcept
        {
                pc = memory->deref_pointer(pc + 1);
        }

        void jsr() noexcept
        {
                stack_push_pointer(pc + 2);
                pc = memory->read_pointer(pc + 1);
        }

        void lda(Byte operand) noexcept
        {
                transfer(a, operand);
        }

        void ldx(Byte operand) noexcept
        {
                transfer(x, operand);
        }

        void ldy(Byte operand) noexcept
        {
                transfer(y, operand);
        }

        Byte lsr(Byte operand) noexcept
        {
                Byte const result = operand >> 1;

                p.set(carry_flag, Utils::bit(operand, 0));
                update_transfer_flags(result);

                return result;
        }

        void nop() noexcept
        {

        void ora(CPU& cpu, Byte operand) noexcept
        {
                transfer(a, a | operand);
        }

        void pha() noexcept
        {
                stack_push_byte(a);
        }

        void php() noexcept
        {
                stack_push_byte(Utils::to_byte(p));
        }

        void pla() noexcept
        {
                transfer(a, stack_pull_byte());
        }

        void plp() noexcept
        {
                p = stack_pull_byte();
                p.set(unused_flag);
        }

        Byte rol(Byte operand) noexcept
        {
                Byte const result = [&] {
                        ByteBitset bits(operand);
                        bits <<= 1;
                        bits.set(0, p.test(carry_flag));
                        return bits.to_ulong();
                }();

                status(carry_flag, Utils::bit(operand, sign_bit));
                update_transfer_flags(result);
                return result;
        }

        Byte ror(Byte operand) noexcept
        {
                Byte const result = [&] {
                        ByteBitset bits(operand);
                        bits >>= 1;
                        bits.set(sign_bit, p.test(carry_flag));
                        return bits.to_ulong();
                }();

                p.set(carry_flag, Utils::bit(operand, 0));
                update_transfer_flags(result);
                return result;
        }

        void brk() noexcept
        {
                if (p.test(interrupt_disable_flag))
                        return;

                stack_push_pointer(pc + 2);
                cpu.set(break_flag);
                stack_push_byte(p.to_ulong());
                cpu.set(break_flag, false);
                cpu.set(interrupt_disable_flag);
                load_interrupt_handler(Interrupt::irq);
        }

        void rti() noexcept
        {
                p = stack_pull_byte();
                p.set(break_flag, false);
                pc = stack_pull_pointer();
        }

        void rts() noexcept
        {
                pc = stack_pull_pointer() + 1;
        }

        void sec() noexcept
        {
                p.set(carry_flag);
        }

        void sei() noexcept
        {
                p.set(interrupt_disable_flag);
        }

        Byte sta() noexcept
        {
                return a;
        }

        Byte stx() noexcept
        {
                return x;
        }

        Byte sty() noexcept
        {
                return y;
        }

        void tax() noexcept
        {
                transfer(x, a);
        }

        void tay() noexcept
        {
                transfer(y, a);
        }

        void tsx() noexcept
        {
                transfer(x, sp);
        }

        void txa() noexcept
        {
                transfer(a, x);
        }

        void txs() noexcept
        {
                transfer(sp, x);
        }

        void tya() noexcept
        {
                transfer(a, y);
        }

        Instruction translate_opcode(Byte opcode)
        {
                switch (opcode) {
                        case 0x00: return brk;
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
                        case 0x20: return jsr;
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
                        case 0x60: return rts;
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

        UniqueMemory memory;
        unsigned pc = 0;
        Byte sp = byte_max;
        Byte a = 0;
        Byte x = 0;
        Byte y = 0;
        ByteBitset p = 0x20;
};

CPU::CPU(UniqueMemory memory) noexcept
        : impl_(std::make_unique<Impl>(std::move(memory)))
{}

CPU::~CPU() = default;

unsigned CPU::pc() const noexcept
{
        return impl_->pc;
}

Byte CPU::sp() const noexcept
{
        return impl_->sp;
}

Byte CPU::a() const noexcept
{
        return impl_->a;
}

Byte CPU::x() const noexcept
{
        return impl_->x;
}

Byte CPU::y() const noexcept
{
        return impl_->y;
}

Byte CPU::p() const noexcept
{
        return impl_->p;
}

void CPU::execute_instruction()
{
        auto const opcode = memory->read_byte(pc);
        auto const instruction = translate_opcode(opcode);
        instruction();
}

void CPU::hardware_interrupt(Interrupt interrupt)
{
        if (interrupt == Interrupt::reset) {
                impl_ = std::make_unique<Impl>(impl_->memory.cartridge,
                                               impl_->memory.ppu);
                return;
        }

        if (interrupt == Interrupt::irq && p.test(interrupt_disable_flag))
                return;

        impl_->stack_push_pointer(pc);
        impl_->stack_push_byte(p.to_ulong());
        p.set(interrupt_disable_flag);
        load_interrupt_handler(interrupt);
}

}

