#include "cpu.h"
#include <utility>
#include <string>
#include <sstream>

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
        : runtime_error(error_message(opcode))
{}

std::string UnknownOpcode::error_message(Byte opcode) noexcept
{
        std::stringstream ss;
        ss << "Unknown opcode " << Utils::format_hex(opcode) << ".";
        return ss.str();
}

void CPU::RAM::write_byte(unsigned address, Byte byte)
{
        ram_[address % real_size] = byte;
}

Byte CPU::RAM::read_byte(unsigned address) const
{
        return ram_[address % real_size];
}

unsigned CPU::interrupt_handler_address(Interrupt interrupt) noexcept
{
        switch (interrupt) {
                case Interrupt::nmi:   return 0xFFFA;
                case Interrupt::reset: return 0xFFFC;
                case Interrupt::irq:   return 0xFFFE;
                default:               return 0x0000;
        }
}

void CPU::execute_program(unsigned program_size)
{
        unsigned const start = pc;
        while (pc != start + program_size)
                execute_instruction();
}

void CPU::execute_instruction()
{
        auto const old_a = a;
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

void CPU::raise_interrupt(Interrupt interrupt)
{
        Stack::push_pointer(*this, pc);
        Stack::push_byte(*this, Utils::to_byte(p));
        status(CPU::break_flag, false);
        status(interrupt_disable_flag, true);
        pc = interrupt_handler(interrupt);
}

unsigned CPU::interrupt_handler(Interrupt interrupt) noexcept
{
        unsigned const pointer_address = interrupt_handler_address(interrupt);
        return memory->read_pointer(pointer_address);
}

}

