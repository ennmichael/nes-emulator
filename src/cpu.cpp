#include "cpu.h"
#include <utility>
#include <string>
#include <sstream>
#include <cassert>
#include <algorithm>

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

CPU::AccessibleMemory::AccessibleMemory(Pieces pieces) noexcept
        : pieces_(std::move(pieces))
{}

bool CPU::AccessibleMemory::address_is_writable(unsigned address) const noexcept
{
        return std::any_of(pieces_.cbegin(), pieces_.cend(),
                           [&](Memory* piece)
                           { return piece->address_is_writable(address); });
}

bool CPU::AccessibleMemory::address_is_readable(unsigned address) const noexcept
{
        return std::any_of(pieces_.cbegin(), pieces_.cend(),
                           [&](Memory* piece)
                           { return piece->address_is_readable(address); });
}

void CPU::AccessibleMemory::write_byte(unsigned address, Byte byte)
{
        find_writable_piece(address).write_byte(address, byte);
}

Byte CPU::AccessibleMemory::read_byte(unsigned address)
{
        return find_readable_piece(address).read_byte(address);
}

Memory& CPU::AccessibleMemory::find_writable_piece(unsigned address)
{
        return find_piece([&](Memory* piece)
                          { return piece->address_is_writable(address); },
                          [&]
                          {
                                  return "CPU can't write to address "s +
                                         Utils::format_address(address) + "."s;
                          });
}

Memory& CPU::AccessibleMemory::find_readable_piece(unsigned address)
{
        return find_piece([&](Memory* piece)
                          { return piece->address_is_readable(address); },
                          [&]
                          {
                                  return "CPU can't read address "s +
                                         Utils::format_address(address) + "."s;
                          });
}

struct CPU::Impl {
        explicit Impl(std::unique_ptr<AccessibleMemory> memory)
                : memory(std::move(memory))
        {
                load_interrupt_handler(Interrupt::reset);
        }

        explicit Impl(AccessibleMemory::Pieces memory_pieces)
                : memory(std::make_unique<AccessibleMemory>(memory_pieces))
        {}

        unsigned stack_top_address() const noexcept
        {
                return stack_bottom_address + sp;
        }

        Byte stack_top_byte()
        {
                return memory->read_byte(stack_top_address() + 1);
        }

        unsigned stack_top_pointer()
        {
                return memory->read_pointer(stack_top_address() + 1);
        }

        void stack_push_byte(Byte byte)
        {
                memory->write_byte(stack_top_address(), byte);
                sp -= 1;
        }

        void stack_push_pointer(unsigned pointer)
        {
                memory->write_pointer(stack_top_address() - 1, pointer);
                sp -= address_size;
        }

        Byte stack_pull_byte()
        {
                Byte const result = stack_top_byte();
                sp += 1;
                return result;
        }

        unsigned stack_pull_pointer() noexcept
        {
                unsigned const result = stack_top_pointer();
                sp += address_size;
                return result;
        }

        unsigned interrupt_handler(Interrupt interrupt) noexcept
        {
                unsigned const pointer_address = interrupt_handler_address(interrupt);
                return memory->read_pointer(pointer_address);
        }

        void load_interrupt_handler(Interrupt interrupt) noexcept
        {
                pc = interrupt_handler(interrupt);
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
                        (this->*operation)();
                        pc += 1;
                };
        }

        template <class Operation>
        Instruction accumulator(Operation operation)
        {
                return [this, operation]
                {
                        a = (this->*operation)(a);
                        pc += 1;
                };
        }

        template <class Operation>
        Instruction immediate(Operation operation)
        {
                return [this, operation]
                {
                        auto const operand = memory->read_byte(pc + 1);
                        (this->*operation)(operand);
                        pc += 2;
                };
        }

        template <class Branch>
        Instruction relative(Branch branch)
        {
                return [this, branch]
                {
                        if ((this->*branch)()) {
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
        Instruction indirect_y(Operation operation) // Indirect indexed
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

        void execute_on_memory(Byte (Impl::*operation)(),
                               unsigned address)
        {
                memory->write_byte(address, (this->*operation)());
        }

        void execute_on_memory(void (Impl::*operation)(Byte operand),
                               unsigned address)
        {
                auto const operand = memory->read_byte(address);
                (this->*operation)(operand);
        }

        void execute_on_memory(Byte (Impl::*operation)(Byte operand),
                               unsigned address)
        {
                auto const operand = memory->read_byte(address);
                memory->write_byte(address, (this->*operation)(operand));
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

        void absolute_jsr() noexcept
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
        {}

        void ora(Byte operand) noexcept
        {
                transfer(a, a | operand);
        }

        void pha() noexcept
        {
                stack_push_byte(a);
        }

        void php() noexcept
        {
                stack_push_byte(p.to_ulong());
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

                p.set(carry_flag, Utils::bit(operand, sign_bit));
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

        void implied_brk()
        {
                if (p.test(interrupt_disable_flag))
                        return;

                stack_push_pointer(pc + 2);
                p.set(break_flag);
                stack_push_byte(p.to_ulong());
                p.set(break_flag, false);
                p.set(interrupt_disable_flag);
                load_interrupt_handler(Interrupt::irq);
        }

        void rti() noexcept
        {
                p = stack_pull_byte();
                p.set(break_flag, false);
                pc = stack_pull_pointer();
        }

        void implied_rts() noexcept
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
                auto const mem_f = [&](auto const& f)
                {
                        return [this, f](auto&&... params)
                        {
                                return (this->*f)(
                                        std::forward<decltype(params)>(params)...);
                        };
                };

                switch (opcode) {
                        case 0x00: return mem_f(&Impl::implied_brk);
                        case 0x01: return indirect_x(&Impl::ora);
                        case 0x05: return zero_page(&Impl::ora);
                        case 0x06: return zero_page(&Impl::asl);
                        case 0x08: return implied(&Impl::php);
                        case 0x09: return immediate(&Impl::ora);
                        case 0x0A: return accumulator(&Impl::asl);
                        case 0x0D: return absolute(&Impl::ora);
                        case 0x0E: return absolute(&Impl::asl);
                        case 0x10: return relative(&Impl::bpl);
                        case 0x11: return indirect_y(&Impl::ora);
                        case 0x15: return zero_page_x(&Impl::ora);
                        case 0x16: return zero_page_x(&Impl::asl);
                        case 0x18: return implied(&Impl::clc);
                        case 0x19: return absolute_y(&Impl::ora);
                        case 0x1D: return absolute_x(&Impl::ora);
                        case 0x1E: return absolute_x(&Impl::asl);
                        case 0x20: return mem_f(&Impl::absolute_jsr);
                        case 0x21: return indirect_x(&Impl::bitwise_and);
                        case 0x24: return zero_page(&Impl::bit);
                        case 0x25: return zero_page(&Impl::bitwise_and);
                        case 0x26: return zero_page(&Impl::rol);
                        case 0x28: return implied(&Impl::plp);
                        case 0x29: return immediate(&Impl::bitwise_and);
                        case 0x2A: return accumulator(&Impl::rol);
                        case 0x2C: return absolute(&Impl::bit);
                        case 0x2D: return absolute(&Impl::bitwise_and);
                        case 0x2E: return absolute(&Impl::rol);
                        case 0x30: return relative(&Impl::bmi);
                        case 0x31: return indirect_y(&Impl::bitwise_and);
                        case 0x35: return zero_page_x(&Impl::bitwise_and);
                        case 0x36: return zero_page_x(&Impl::rol);
                        case 0x38: return implied(&Impl::sec);
                        case 0x39: return absolute_y(&Impl::bitwise_and);
                        case 0x3D: return absolute_x(&Impl::bitwise_and);
                        case 0x3E: return absolute_x(&Impl::rol);
                        case 0x40: return implied(&Impl::rti);
                        case 0x41: return indirect_x(&Impl::eor);
                        case 0x45: return zero_page(&Impl::eor);
                        case 0x46: return zero_page(&Impl::lsr);
                        case 0x48: return implied(&Impl::pha);
                        case 0x49: return immediate(&Impl::eor);
                        case 0x4A: return accumulator(&Impl::lsr);
                        case 0x4C: return mem_f(&Impl::absolute_jmp);
                        case 0x4D: return absolute(&Impl::eor);
                        case 0x4E: return absolute(&Impl::lsr);
                        case 0x50: return relative(&Impl::bvc);
                        case 0x51: return indirect_y(&Impl::eor);
                        case 0x55: return zero_page_x(&Impl::eor);
                        case 0x56: return zero_page_x(&Impl::lsr);
                        case 0x58: return implied(&Impl::cli);
                        case 0x59: return absolute_y(&Impl::eor);
                        case 0x5D: return absolute_x(&Impl::eor);
                        case 0x5E: return absolute_x(&Impl::lsr);
                        case 0x60: return mem_f(&Impl::implied_rts);
                        case 0x61: return indirect_x(&Impl::adc);
                        case 0x65: return zero_page(&Impl::adc);
                        case 0x66: return zero_page(&Impl::ror);
                        case 0x68: return implied(&Impl::pla);
                        case 0x69: return immediate(&Impl::adc);
                        case 0x6A: return accumulator(&Impl::ror);
                        case 0x6C: return mem_f(&Impl::indirect_jmp);
                        case 0x6D: return absolute(&Impl::adc);
                        case 0x6E: return absolute(&Impl::ror);
                        case 0x70: return relative(&Impl::bvs);
                        case 0x71: return indirect_y(&Impl::adc);
                        case 0x75: return zero_page_x(&Impl::adc);
                        case 0x76: return zero_page_x(&Impl::ror);
                        case 0x78: return implied(&Impl::sei);
                        case 0x79: return absolute_y(&Impl::adc);
                        case 0x7D: return absolute_x(&Impl::adc);
                        case 0x7E: return absolute_x(&Impl::ror);
                        case 0x81: return indirect_x(&Impl::sta);
                        case 0x84: return zero_page(&Impl::sty);
                        case 0x85: return zero_page(&Impl::sta);
                        case 0x86: return zero_page(&Impl::stx);
                        case 0x88: return implied(&Impl::dey);
                        case 0x8A: return implied(&Impl::txa);
                        case 0x8C: return absolute(&Impl::sty);
                        case 0x8D: return absolute(&Impl::sta);
                        case 0x8E: return absolute(&Impl::stx);
                        case 0x90: return relative(&Impl::bcc);
                        case 0x91: return indirect_y(&Impl::sta);
                        case 0x94: return zero_page_x(&Impl::sty);
                        case 0x95: return zero_page_x(&Impl::sta);
                        case 0x96: return zero_page_y(&Impl::stx);
                        case 0x98: return implied(&Impl::tya);
                        case 0x99: return absolute_y(&Impl::sta);
                        case 0x9A: return implied(&Impl::txs);
                        case 0x9D: return absolute_x(&Impl::sta);
                        case 0xA0: return immediate(&Impl::ldy);
                        case 0xA1: return indirect_x(&Impl::lda);
                        case 0xA2: return immediate(&Impl::ldx);
                        case 0xA4: return zero_page(&Impl::ldy);
                        case 0xA5: return zero_page(&Impl::lda);
                        case 0xA6: return zero_page(&Impl::ldx);
                        case 0xA8: return implied(&Impl::tay);
                        case 0xA9: return immediate(&Impl::lda);
                        case 0xAA: return implied(&Impl::tax);
                        case 0xAC: return absolute(&Impl::ldy);
                        case 0xAD: return absolute(&Impl::lda);
                        case 0xAE: return absolute(&Impl::ldx);
                        case 0xB0: return relative(&Impl::bcs);
                        case 0xB1: return indirect_y(&Impl::lda);
                        case 0xB4: return zero_page_x(&Impl::ldy);
                        case 0xB5: return zero_page_x(&Impl::lda);
                        case 0xB6: return zero_page_y(&Impl::ldx);
                        case 0xB8: return implied(&Impl::clv);
                        case 0xB9: return absolute_y(&Impl::lda);
                        case 0xBA: return implied(&Impl::tsx);
                        case 0xBC: return absolute_x(&Impl::ldy);
                        case 0xBD: return absolute_x(&Impl::lda);
                        case 0xBE: return absolute_y(&Impl::ldx);
                        case 0xC0: return immediate(&Impl::cpy);
                        case 0xC1: return indirect_x(&Impl::cmp);
                        case 0xC4: return zero_page(&Impl::cpy);
                        case 0xC5: return zero_page(&Impl::cmp);
                        case 0xC6: return zero_page(&Impl::dec);
                        case 0xC8: return implied(&Impl::iny);
                        case 0xC9: return immediate(&Impl::cmp);
                        case 0xCA: return implied(&Impl::dex);
                        case 0xCC: return absolute(&Impl::cpy);
                        case 0xCD: return absolute(&Impl::cmp);
                        case 0xCE: return absolute(&Impl::dec);
                        case 0xD0: return relative(&Impl::bne);
                        case 0xD1: return indirect_y(&Impl::cmp);
                        case 0xD5: return zero_page_x(&Impl::cmp);
                        case 0xD6: return zero_page_x(&Impl::dec);
                        case 0xD9: return absolute_y(&Impl::cmp);
                        case 0xDD: return absolute_x(&Impl::cmp);
                        case 0xDE: return absolute_x(&Impl::dec);
                        case 0xE0: return immediate(&Impl::cpx);
                        case 0xE1: return indirect_x(&Impl::sbc);
                        case 0xE4: return zero_page(&Impl::cpx);
                        case 0xE5: return zero_page(&Impl::sbc);
                        case 0xE6: return zero_page(&Impl::inc);
                        case 0xE8: return implied(&Impl::inx);
                        case 0xE9: return immediate(&Impl::sbc);
                        case 0xEA: return implied(&Impl::nop);
                        case 0xEC: return absolute(&Impl::cpx);
                        case 0xED: return absolute(&Impl::sbc);
                        case 0xEE: return absolute(&Impl::inc);
                        case 0xF0: return relative(&Impl::beq);
                        case 0xF1: return indirect_y(&Impl::sbc);
                        case 0xF5: return zero_page_x(&Impl::sbc);
                        case 0xF6: return zero_page_x(&Impl::inc);
                        case 0xF9: return absolute_y(&Impl::sbc);
                        case 0xFD: return absolute_x(&Impl::sbc);
                        case 0xFE: return absolute_x(&Impl::inc);
                        default: throw UnknownOpcode(opcode);
                }
        }

        std::unique_ptr<AccessibleMemory> memory;
        unsigned pc = 0;
        Byte sp = byte_max;
        Byte a = 0;
        Byte x = 0;
        Byte y = 0;
        ByteBitset p = 0x20;
};

CPU::CPU(AccessibleMemory::Pieces pieces)
        : impl_(std::make_unique<Impl>(std::move(pieces)))
{}

CPU::~CPU() = default;

unsigned CPU::interrupt_handler_address(Interrupt interrupt) noexcept
{
        switch (interrupt) {
                case Interrupt::nmi:   return 0xFFFA;
                case Interrupt::reset: return 0xFFFC;
                case Interrupt::irq:   return 0xFFFE;
        }

        assert(false);
        return 0; 
}

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
        return impl_->p.to_ulong();
}

void CPU::execute_instruction()
{
        auto const opcode = impl_->memory->read_byte(impl_->pc);
        auto const instruction = impl_->translate_opcode(opcode);
        instruction();
}

void CPU::hardware_interrupt(Interrupt interrupt)
{
        if (interrupt == Interrupt::reset) {
                impl_ = std::make_unique<Impl>(std::move(impl_->memory));
                return;
        }

        if (interrupt == Interrupt::irq && impl_->p.test(interrupt_disable_flag))
                return;

        impl_->stack_push_pointer(impl_->pc);
        impl_->stack_push_byte(impl_->p.to_ulong());
        impl_->p.set(interrupt_disable_flag);
        impl_->load_interrupt_handler(interrupt);
}

bool CPU::address_is_readable(unsigned address) const noexcept
{
        return impl_->memory->address_is_readable(address);
}

Byte CPU::read_byte(unsigned address)
{
        return impl_->memory->read_byte(address);
}

}

