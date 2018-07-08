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

#include <iostream>

namespace Emulator {

class UnknownOpcode : public std::runtime_error {
public:
        explicit UnknownOpcode(Byte opcode) noexcept;

private:
        static std::string error_message(Byte opcode) noexcept;
};

struct CPU;

namespace Stack {

unsigned constexpr bottom_address = 0x100u;

void push_byte(CPU& cpu, Byte byte) noexcept;
void push_pointer(CPU& cpu, unsigned pointer) noexcept;

Byte pull_byte(CPU& cpu) noexcept;
unsigned pull_pointer(CPU& cpu) noexcept;

}

using Instruction = std::function<void(CPU& cpu)>;

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

        class AccessibleMemory : public Memory {
        public:
                AccessibleMemory(Cartridge& cartridge, PPU& ppu) noexcept;

                void write_byte(unsigned address, Byte byte) override;
                Byte read_byte(unsigned address) const override;

        private:
                CPU::RAM ram_;
                Cartridge* cartridge_;
                PPU* ppu_;
        };

        enum class Interrupt {
                irq,
                nmi,
                reset
        };

        static unsigned interrupt_handler_address(Interrupt interrupt) noexcept;

        static unsigned constexpr address_size = 2u;

        static unsigned constexpr carry_flag = 0u;
        static unsigned constexpr zero_flag = 1u;
        static unsigned constexpr interrupt_disable_flag = 2u;
        static unsigned constexpr break_flag = 4u;
        static unsigned constexpr unused_flag = 5u;
        static unsigned constexpr overflow_flag = 6u;
        static unsigned constexpr negative_flag = 7u;  

        static Instruction translate_opcode(Byte opcode);

        UniqueMemory memory;
        unsigned pc = 0;
        Byte sp = byte_max;
        Byte a = 0;
        Byte x = 0;
        Byte y = 0;
        ByteBitset p = 0x20;

        void execute_program(unsigned program_size);
        void execute_instruction();

        bool status(unsigned flag) const noexcept;
        void status(unsigned flag, bool value) noexcept;

        void raise_interrupt(Interrupt interrupt);
        unsigned interrupt_handler(Interrupt interrupt) noexcept;
};

}

