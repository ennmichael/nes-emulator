#pragma once

#include "utils.h"
#include "ppu.h"
#include "cartridge.h"
#include "mem.h"
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
 * Maybe have a struct Registers and pass that into the instructions, and
 * inject the instruction set into the CPU?
 */

namespace Emulator {

class UnknownOpcode : std::runtime_error {
public:
        explicit UnknownOpcode(Byte opcode) noexcept;
};

struct CPU;

using Instruction = std::function<void(CPU& cpu,
                                       Memory& memory,
                                       Bytes const& program)>;

struct CPU {
public:
        class RAM : public Memory {
        public:
                static unsigned constexpr addressable_size = 0x2000u;
                static unsigned constexpr real_size = 0x800u;
                static unsigned constexpr mirrors_size = addressable_size - real_size;

                void write_byte(unsigned address, Byte byte) override;
                Byte read_byte(unsigned address) const override;

        private:
                std::array<Byte, real_size> ram_ {0};
        };

        class Stack {
        public:
                static unsigned constexpr bottom_address = 0x100u;

                Stack(RAM& ram, Byte& sp) noexcept;

                void push_byte(Byte byte) noexcept;
                void push_pointer(unsigned pointer) noexcept;

                Byte pull_byte() noexcept;
                unsigned pull_pointer() noexcept;

        private:
                unsigned absolute_top_address() const noexcept;

                Byte top_byte() const noexcept;
                unsigned top_pointer() const noexcept;

                RAM* ram_;
                Byte* sp_;
        };

        enum class Interrupt {
                irq,
                nmi,
                reset
        };

        static unsigned handler_pointer_address(Interrupt interrupt) noexcept;

        static unsigned constexpr address_size = 2u;

        static unsigned constexpr carry_flag = 0u;
        static unsigned constexpr zero_flag = 1u;
        static unsigned constexpr interrupt_disable_flag = 2u;
        static unsigned constexpr break_flag = 4u;
        static unsigned constexpr overflow_flag = 6u;
        static unsigned constexpr negative_flag = 7u;  

        static Instruction translate_opcode(Byte opcode);

        unsigned pc = 0;
        Byte sp = byte_max;
        Byte a = 0;
        Byte x = 0;
        Byte y = 0;
        ByteBitset p = 0x34;
        RAM ram;
        Stack stack = Stack(ram, sp);

        void execute_program(Memory& memory, unsigned program_size);
        void execute_program(Cartridge& cartridge, PPU& ppu);

        void execute_instruction(Memory& memory);

        bool status(unsigned flag) const noexcept;
        void status(unsigned flag, bool value) noexcept;

        void raise_interrupt(Interrupt interrupt);
        unsigned handler_pointer(Interrupt interrupt) noexcept;
};

class NESMemory : public Memory {
public:
        NESMemory(CPU::RAM& ram, Cartridge& cartridge, PPU& ppu) noexcept;

        void write_byte(unsigned address, Byte byte) override;
        Byte read_byte(unsigned address) const override;

private:
        CPU::RAM* ram_;
        Cartridge* cartridge_;
        PPU* ppu_;
};

}

