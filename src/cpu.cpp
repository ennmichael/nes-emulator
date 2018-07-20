#include "cpu.h"
#include <utility>
#include <string>
#include <sstream>
#include <cassert>

// CPU::translate_opcode is defined in instruction_set.cpp

using namespace std::string_literals;

namespace Emulator {

namespace Stack {

namespace {

unsigned top_address(CPU const& cpu) noexcept
{
        return bottom_address + cpu.sp;
}

Byte top_byte(CPU const& cpu) noexcept
{
        return cpu.memory->read_byte(top_address(cpu) + 1);
}

unsigned top_pointer(CPU const& cpu) noexcept
{
        return cpu.memory->read_pointer(top_address(cpu) + 1);
}

}

void push_byte(CPU& cpu, Byte byte) noexcept
{
        cpu.memory->write_byte(top_address(cpu), byte);
        cpu.sp -= 1;
}

void push_pointer(CPU& cpu, unsigned pointer) noexcept
{
        cpu.memory->write_pointer(top_address(cpu) - 1, pointer);
        cpu.sp -= CPU::address_size;
}

Byte pull_byte(CPU& cpu) noexcept
{
        Byte const result = top_byte(cpu);
        cpu.sp += 1;
        return result;
}

unsigned pull_pointer(CPU& cpu) noexcept
{
        unsigned const result = top_pointer(cpu);
        cpu.sp += CPU::address_size;
        return result;
}

}

UnknownOpcode::UnknownOpcode(Byte opcode) noexcept
        : runtime_error("Unknown opcode "s + Utils::format_hex(opcode, 2) + "."s)
{}

bool CPU::RAM::address_is_accessible(unsigned address) noexcept
{
        return start <= address && address < end;
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

void CPU::execute_program()
{
        load_interrupt_handler(Interrupt::reset);
        for (;;)
                execute_instruction();
}

void CPU::execute_instruction()
{
        auto const opcode = memory->read_byte(pc);
        auto const instruction = translate_opcode(opcode);
        instruction(*this);
}

bool CPU::status(unsigned flag) const noexcept
{
        return p.test(flag);
}

void CPU::status(unsigned flag, bool value) noexcept
{
        p.set(flag, value);
}

void CPU::hardware_interrupt(Interrupt interrupt)
{
        if (interrupt == Interrupt::reset) {
                *this = CPU();
                execute_program();
                return;
        }

        if (interrupt == Interrupt::irq && status(interrupt_disable_flag))
                return;

        Stack::push_pointer(*this, pc);
        Stack::push_byte(*this, Utils::to_byte(p));
        status(interrupt_disable_flag, true);
        load_interrupt_handler(interrupt);
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

}

