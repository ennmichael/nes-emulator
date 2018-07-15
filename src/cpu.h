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
                static unsigned constexpr start = 0x0000u;
                static unsigned constexpr end = 0x2000u;
                static unsigned constexpr real_size = 0x0800u;
                static unsigned constexpr mirrors_size = end - real_size;

                bool address_is_writable(unsigned address) const noexcept override;
                bool address_is_readable(unsigned address) const noexcept override;

        private:
                void do_write_byte(unsigned address, Byte byte) override;
                Byte do_read_byte(unsigned address) const override;

                unsigned translate_address(unsigned address) const noexcept;

                std::array<Byte, real_size> ram_ {0};
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

        void execute_program();
        void execute_instruction();

        bool status(unsigned flag) const noexcept;
        void status(unsigned flag, bool value) noexcept;

        void hardware_interrupt(Interrupt interrupt);
        unsigned interrupt_handler(Interrupt interrupt) const noexcept;
        void load_interrupt_handler(Interrupt interrupt) noexcept;
};

}

