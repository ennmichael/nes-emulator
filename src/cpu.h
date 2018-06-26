#pragma once

#include "utils.h"
#include "ppu.h"
#include "cartridge.h"
#include <array>
#include <utility>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
#include <stdexcept>

/**
 * Ways CPU could be refactored:
 * The instructions could be made more expressive.
 * I wish I could move the instructions out of the class, still have them work,
 * and keep the registers encapsulated (i.e. CPU doesn't become a struct).
 * This could be done via return values from the instructions?
 * The most important thing might be to fix the repetition between some of the
 * instructions' definitions in an intuitive way that doesn't suck.
 * [LDA, LDX, etc.], [TAX, TAY, TXA, etc.]
 */

namespace Emulator {

class UnknownOpcode : std::runtime_error {
public:
        explicit UnknownOpcode(Byte opcode) noexcept;
};

// TODO Rename read -> read_byte, write -> write_byte ???????

class CPU {
public:
        class Memory {
        public:
                static unsigned constexpr adressable_size = 0xFFFF;
                static unsigned constexpr internal_ram_size = 0x800;
                static unsigned constexpr ram_mirrors_size =
                        0x2000 - internal_ram_size;

                Memory(Cartridge& cartridge, PPU& ppu) noexcept;

                void write(unsigned address, Byte byte) noexcept;
                Byte read(unsigned address) const noexcept;
                unsigned read_address(unsigned pointer) const noexcept;

        private:
                Cartridge* cartridge_;
                PPU* ppu_;
                std::array<Byte, internal_ram_size> ram_ {0};
        }; 
        
        // We'll probably have a Stack class taking a reference to Memory?
        // Also taking a reference to sp_?

        static unsigned constexpr carry_flag = 0;
        static unsigned constexpr zero_flag = 1;
        static unsigned constexpr interrupt_disable_flag = 2;
        static unsigned constexpr break_flag = 4;
        static unsigned constexpr overflow_flag = 6;
        static unsigned constexpr negative_flag = 7;  

        CPU(Cartridge& cartridge, PPU& ppu) noexcept; 

        void execute_program(Bytes const& program);
        void execute_instruction(Bytes const& program);

        bool status(unsigned flag) const noexcept;

        Byte read(unsigned address) const noexcept;
        unsigned read_address(unsigned pointer) const noexcept;

private:
        using Instruction = std::function<void(CPU& cpu, Bytes const& program)>;

        static Instruction translate_opcode(Byte opcode) noexcept;

        // Example of instructions that only store stuff in memory: STA, STX, etc.
        template <class Operation>
        static void store_in_memory(Operation operation, CPU& cpu, unsigned address);
        // Examples of instructions that operate in-place: INC, DEC, ASL, etc.
        template <class Operation>
        static void execute_on_memory_in_place(Operation operation,
                                               Byte operand,
                                               CPU& cpu,
                                               unsigned address);
        template <class Operation>
        static void execute_on_memory(Operation operation,
                                      CPU& cpu, unsigned address);

        unsigned fetch_absolute_address(Bytes const& program) const noexcept;
        unsigned fetch_indirect_address(Bytes const& program) const noexcept;

        template <class Operation, class Offset>
        static Instruction zero_page(Operation operation, Offset offset);
        template <class Operation>
        static Instruction zero_page(Operation operation);
        template <class Operation>
        static Instruction zero_page_x(Operation operation);
        template <class Operation>
        static Instruction zero_page_y(Operation operation);
        template <class Operation, class Offset>
        static Instruction absolute(Operation operation, Offset offset)
        template <class Operation>
        static Instruction absolute(Operation operation)
        template <class Operation>
        static Instruction absolute_x(Operation operation)
        template <class Operation>
        static Instruction absolute_y(OperationCallback const& operation)
        template <class Operation>
        static Instruction indirect(Operation operation)
        template <class Operation>
        static Instruction implied(Operation operation)
        template <class Operation>
        static Instruction accumulator(Operation operation)
        template <class Operation>
        static Instruction immediate(Operation operation)
        template <class Branch>
        static Instruction relative(Branch branch)
        template <class Operation>
        static Instruction indirect_x(Operation operation) // Indexed indirect
        template <class Operation>
        static Instruction indirect_y(Operation operation) // Indirect indexed

        static bool bcs(CPU const& cpu) noexcept;
        static bool bcc(CPU const& cpu) noexcept;
        static bool beq(CPU const& cpu) noexcept;
        static bool bne(CPU const& cpu) noexcept;
        static bool bmi(CPU const& cpu) noexcept;
        static bool bpl(CPU const& cpu) noexcept;
        static bool bvs(CPU const& cpu) noexcept;
        static bool bvc(CPU const& cpu) noexcept;

        static void adc(CPU& cpu, Byte operand) noexcept;
        static void sbc(CPU& cpu, Byte operand) noexcept;

        static void bitwise_and(CPU& cpu, Byte operand) noexcept;

        static Byte asl(CPU& cpu, Byte operand) noexcept;
        static void bit(CPU& cpu, Byte operand) noexcept;
        static void brk(CPU& cpu, Byte operand) noexcept;
        static void clc(CPU& cpu, Byte operand) noexcept;
        static void cli(CPU& cpu, Byte operand) noexcept;
        static void clv(CPU& cpu, Byte operand) noexcept;
        static void cmp(CPU& cpu, Byte operand) noexcept;
        static void cpx(CPU& cpu, Byte operand) noexcept;
        static void cpy(CPU& cpu, Byte operand) noexcept;
        static Byte dec(CPU& cpu, Byte operand) noexcept;
        static void dex(CPU& cpu, Byte operand) noexcept;
        static void dey(CPU& cpu, Byte operand) noexcept;
        static void eor(CPU& cpu, Byte operand) noexcept;
        static Byte inc(CPU& cpu, Byte operand) noexcept;
        static void inx(CPU& cpu, Byte operand) noexcept;
        static void iny(CPU& cpu, Byte operand) noexcept;
        static void absolute_jmp(CPU& cpu, Bytes const& program) noexcept;
        static void indirect_jmp(CPU& cpu, Bytes const& program) noexcept;
        static void jsr(CPU& cpu, Byte operand) noexcept;

        static void lda(CPU& cpu, Byte operand) noexcept;
        static void ldx(CPU& cpu, Byte operand) noexcept;
        static void ldy(CPU& cpu, Byte operand) noexcept;

        static Byte lsr(CPU& cpu, Byte operand) noexcept;
        static void nop(CPU& cpu, Byte operand) noexcept;
        static void ora(CPU& cpu, Byte operand) noexcept;
        static void pha(CPU& cpu, Byte operand) noexcept;
        static void php(CPU& cpu, Byte operand) noexcept;
        static void pla(CPU& cpu, Byte operand) noexcept;
        static void plp(CPU& cpu, Byte operand) noexcept;
        static Byte rol(CPU& cpu, Byte operand) noexcept;
        static Byte ror(CPU& cpu, Byte operand) noexcept;
        static void rti(CPU& cpu, Byte operand) noexcept;
        static void rts(CPU& cpu, Byte operand) noexcept;
        static void sec(CPU& cpu, Byte operand) noexcept;
        static void sei(CPU& cpu, Byte operand) noexcept;
        static Byte sta(CPU& cpu) noexcept;
        static Byte stx(CPU& cpu) noexcept;
        static Byte sty(CPU& cpu) noexcept;
        static void tax(CPU& cpu, Byte operand) noexcept;
        static void tay(CPU& cpu, Byte operand) noexcept;
        static void tsx(CPU& cpu, Byte operand) noexcept;
        static void txa(CPU& cpu, Byte operand) noexcept;
        static void txs(CPU& cpu, Byte operand) noexcept;
        static void tya(CPU& cpu, Byte operand) noexcept;
        static void compare(CPU& cpu, Byte reg, Byte operand) noexcept;
        
        void write(unsigned address, Byte byte) noexcept;
        void status(unsigned flag, bool value) noexcept;

        template <class IntegerType>
        void update_zero_flag(IntegerType i) noexcept;
        void update_overflow_flag(unsigned old_value, unsigned result) noexcept;
        void update_negative_flag(unsigned result) noexcept;

        Memory memory_;
        unsigned pc_ = 0;
        Byte sp_ = 0;
        // We don't need to have sp_ as a register, we can have a Stack class
        Byte a_ = 0;
        Byte x_ = 0;
        Byte y_ = 0;
        ByteBitset p_ = 0x34; // TODO This doesn't seem correct
};

template <class Operation>
void CPU::execute_on_memory_in_place(Operation operation,
                                     Byte operand,
                                     CPU& cpu,
                                     unsigned address)
{
        Byte const result = operation(cpu, operand);
        cpu.write(address, result);
}

template <class Operation>
void CPU::store_in_memory(Operation operation, CPU& cpu, unsigned address)
{
        cpu.write(address, operation(cpu));
}

template <class Operation>
void CPU::execute_on_memory(Operation operation, CPU& cpu, unsigned address)
{
        if constexpr (std::is_invocable_v<Operation, CPU&>) {
                store_in_memory(operation, cpu, address);
                return;
        }

        Byte const operand = cpu.read_memory(address);

        if constexpr (Utils::returns_void<OperationCallback, CPU&, unsigned>) {
                operation(cpu, operand);
        } else {
                execute_on_memory_in_place(operation, operand, cpu, address);
        }
}

template <class Operation, class Offset>
Instruction CPU::zero_page(Operation operation, Offset offset)
{
        return [operation, offset](CPU& cpu, Bytes const& program)
        {
                Byte const base_address = program[cpu.pc + 1];
                unsigned const address = base_address + offset(cpu);
                execute_on_memory(operation, cpu, address);
                cpu.pc_ += 2;
        };
}

template <class Operation>
Instruction CPU::zero_page(Operation operation)
{
        return zero_page(operation, [](CPU const&) { return 0; });
}

template <class Operation>
Instruction CPU::zero_page_x(Operation operation)
{
        return zero_page(operation, [](CPU const& cpu) { return cpu.x_; });
}

template <class Operation>
Instruction CPU::zero_page_y(Operation operation)
{
        return zero_page(operation, [](CPU const& cpu) { return cpu.y_; });
}

template <class Operation, class Offset>
Instruction CPU::absolute(Operation operation, Offset offset)
{
        return [operation, offset](CPU& cpu, Bytes const& program)
        {
                unsigned const address =
                        cpu.fetch_absolute_address(program) + offset(cpu);
                execute_on_memory(operation, cpu, address);
                cpu.pc_ += 3;
        };
}

template <class Operation>
Instruction CPU::absolute(Operation operation)
{
        return absolute(operation, [](CPU const&) { return 0; });
}

template <class Operation>
Instruction CPU::absolute_x(Operation operation)
{
        return absolute(operation, [](CPU const& cpu) { return cpu.x_; });
}

template <class Operation>
Instruction CPU::absolute_y(OperationCallback const& operation)
{
        return absolute(operation, [](CPU const& cpu) { return cpu.y_; });
}

template <class Operation>
Instruction CPU::indirect(Operation operation)
{
        return [operation](CPU& cpu, Bytes const& program)
        {
                unsigned const address = cpu.fetch_indirect_address(program);
                execute_on_memory(operation, cpu, address);
                cpu.pc_ += 3;
        };
}

template <class Operation>
Instruction CPU::implied(Operation operation)
{
        return [operation](CPU& cpu, Bytes const&)
        {
                operation(cpu);
                cpu.pc_ += 1;
        };
}

template <class Operation>
Instruction CPU::accumulator(Operation operation)
{
        return [operation](CPU& cpu, Bytes const&)
        {
                cpu.a = operation(cpu, cpu.a);
                cpu.pc_ += 1;
        };
}

template <class Operation>
Instruction CPU::immediate(Operation operation)
{
        return [operation](CPU& cpu, Bytes const& program)
        {
                Byte const operand = program[cpu.pc + 1];
                operation(cpu, operand);
                cpu.pc_ += 2;
        };
}

template <class Branch>
Instruction CPU::relative(Branch branch)
{
        return [operation](CPU& cpu, Bytes const& program)
        {
                auto const delta = static_cast<int>(program[cpu.pc_ + 1]);
                if (branch(cpu))
                        cpu.pc_ += delta;
                cpu.pc_ += 2;
        };
}

template <class Operation>
Instruction CPU::indirect_x(Operation operation) // Indexed indirect
{
        return [operation](CPU& cpu, Bytes const& program)
        {
                unsigned const pointer = program[cpu.pc_ + 1] + cpu.x_;
                unsigned const address = cpu.memory.read_address(pointer);
                execute_on_memory(operation, cpu, address);
                cpu.pc_ += 2;
        };
}

template <class Operation>
Instruction CPU::indirect_y(Operation operation) // Indirect indexed
{
        return [operation](CPU& cpu, Bytes const& program)
        {
                unsigned const pointer = program[cpu.pc_ + 1];
                unsigned const address = cpu.read_address(pointer) + cpu.y_;
                execute_on_memory(operation, cpu, address);
                cpu.pc_ += 2;
        };
}

template <class IntegerType>
void CPU::update_zero_flag(IntegerType i) noexcept
{
        status(zero_flag, i == 0);
}

}

