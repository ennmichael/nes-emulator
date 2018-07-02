#include "cpu.h"
#include <utility>
#include <string>

// CPU::translate_opcode is defined in instruction_set.cpp

using namespace std::string_literals;

namespace Emulator {

UnknownOpcode::UnknownOpcode(Byte opcode) noexcept
        : runtime_error("Unknown opcode "s + std::to_string(opcode))
{}

void CPU::RAM::write_byte(unsigned address, Byte byte)
{
        ram_[address % real_size] = byte;
}

Byte CPU::RAM::read_byte(unsigned address) const
{
        return ram_[address % real_size];
}

CPU::Stack::Stack(RAM& ram, Byte& sp) noexcept
        : ram_(&ram)
        , sp_(&sp)
{}

void CPU::Stack::push_byte(Byte byte) noexcept
{
        ram_->write_byte(absolute_top_address(), byte);
        *sp_ -= 1;
}

void CPU::Stack::push_pointer(unsigned pointer) noexcept
{
        ram_->write_pointer(absolute_top_address() - 1, pointer);
        *sp_ -= address_size;
}

Byte CPU::Stack::pull_byte() noexcept
{
        Byte const result = top_byte();
        *sp_ += 1;
        return result;
}

unsigned CPU::Stack::pull_pointer() noexcept
{
        unsigned const result = top_pointer();
        *sp_ += address_size;
        return result;
}

unsigned CPU::Stack::absolute_top_address() const noexcept
{
        return bottom_address + *sp_;
}

Byte CPU::Stack::top_byte() const noexcept
{
        return ram_->read_byte(absolute_top_address() + 1);
}

unsigned CPU::Stack::top_pointer() const noexcept
{
        return ram_->read_pointer(absolute_top_address() + 1);
}

unsigned CPU::handler_pointer_address(Interrupt interrupt) noexcept
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
        execute_program(ram, program_size);
}

void CPU::execute_program(Memory& memory, unsigned program_size)
{
        unsigned const start = pc;
        while (pc != start + program_size)
                execute_instruction(memory);
}

void CPU::execute_program(Cartridge& cartridge, PPU& ppu)
{}

void CPU::execute_instruction(Memory& memory)
{
        auto const opcode = memory.read_byte(pc);
        auto const instruction = translate_opcode(opcode);
        instruction(*this, memory);
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
        stack.push_pointer(pc);
        stack.push_byte(Utils::to_byte(p));
        status(interrupt_disable_flag, true);
        pc = handler_pointer(interrupt);
}

unsigned CPU::handler_pointer(Interrupt interrupt) noexcept
{
        unsigned const pointer_address = handler_pointer_address(interrupt);
        return ram.read_pointer(pointer_address);
}

}

