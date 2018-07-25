#pragma once

#include "utils.h"
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

class CPU : ReadableMemory {
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

        private:
                unsigned translate_address(unsigned address) const noexcept;

                ByteArray<real_size> ram_ {0};
        };

        class AccessibleMemory : public Memory {
        public:
                using Pieces = std::vector<Memory*>;

                AccessibleMemory(Pieces pieces) noexcept;
                bool address_is_writable(unsigned address) const noexcept override;
                bool address_is_readable(unsigned address) const noexcept override;
                void write_byte(unsigned address, Byte byte) override;
                Byte read_byte(unsigned address) override;

        private:
                Memory& find_writable_piece(unsigned address);
                Memory& find_readable_piece(unsigned address);

                template <class P, class E>
                Memory& find_piece(P const& p, E const& e)
                {
                        auto const i = std::find_if(pieces_.cbegin(),
                                                    pieces_.cend(), p);
                        if (i == pieces_.cend())
                                throw InvalidAddress(e());
                        return *i;
                }

                Pieces pieces_;
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

        explicit CPU(AccessibleMemory::Pieces pieces);
        CPU(CPU const& other) = delete;
        CPU(CPU&& other) = default;
        CPU& operator=(CPU const& other) = delete;
        CPU& operator=(CPU&& other) = default;
        ~CPU();

        static unsigned constexpr stack_bottom_address = 0x100u;

        static unsigned interrupt_handler_address(Interrupt interrupt) noexcept;

        unsigned pc() const noexcept;
        Byte sp() const noexcept;
        Byte a() const noexcept;
        Byte x() const noexcept;
        Byte y() const noexcept;
        Byte p() const noexcept;
        void execute_instruction();
        void hardware_interrupt(Interrupt interrupt);
        bool address_is_readable(unsigned address) const noexcept override;
        Byte read_byte(unsigned address) override;

private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
};

using UniqueCPU = std::unique_ptr<CPU>;

}

