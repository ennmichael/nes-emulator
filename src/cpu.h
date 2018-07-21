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

namespace Emulator {

class UnknownOpcode : public std::runtime_error {
public:
        explicit UnknownOpcode(Byte opcode) noexcept;
};

using Instruction = std::function<void()>;

class CPU {
public:
        class RAM : public Memory {
        public:
                static unsigned constexpr start = 0x0000u;
                static unsigned constexpr end = 0x2000u;
                static unsigned constexpr real_size = 0x0800u;
                static unsigned constexpr mirrors_size = end - real_size;

                static bool address_is_accessible(unsigned addres) noexcept;
                bool address_is_writable(unsigned address) const noexcept override;
                bool address_is_readable(unsigned address) const noexcept override;

                void write_byte(unsigned address, Byte byte) override;
                Byte read_byte(unsigned address) override;
                Byte read_byte(unsigned address) const;

        private:
                unsigned translate_address(unsigned address) const noexcept;

                std::array<Byte, real_size> ram_ {0};
        };

        enum class Interrupt {
                irq,
                nmi,
                reset
        };

        static unsigned constexpr address_size = 2u;

        static unsigned constexpr carry_flag = 0u;
        static unsigned constexpr zero_flag = 1u;
        static unsigned constexpr interrupt_disable_flag = 2u;
        static unsigned constexpr break_flag = 4u;
        static unsigned constexpr unused_flag = 5u;
        static unsigned constexpr overflow_flag = 6u;
        static unsigned constexpr negative_flag = 7u;  

        explicit CPU(Memory& cartridge_memory, Memory& ppu_memory) noexcept;
        CPU(CPU const& other) = delete;
        CPU(CPU&& other) = default;
        CPU& operator=(CPU const& other) = delete;
        CPU& operator=(CPU&& other) = default;
        ~CPU();

        unsigned constexpr stack_bottom_address = 0x100u;

        unsigned pc() const noexcept;
        Byte sp() const noexcept;
        Byte a() const noexcept;
        Byte x() const noexcept;
        Byte y() const noexcept;
        Byte p() const noexcept;
        void execute_instruction();
        void hardware_interrupt(Interrupt interrupt);

private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
};

}

