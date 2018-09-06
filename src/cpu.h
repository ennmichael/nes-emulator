#pragma once

#include "utils.h"
#include <cassert>
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

class CPU : public ReadableMemory {
public:
        class RAM : public Memory {
        public:
                static Address constexpr start = 0x0000;
                static Address constexpr end = 0x1FFF;
                static Address constexpr real_size = 0x0800;

                static bool address_is_accessible(Address addres) noexcept;

        protected:
                bool address_is_writable_impl(Address address) const noexcept override;
                bool address_is_readable_impl(Address address) const noexcept override;
                void write_byte_impl(Address address, Byte byte) override;
                Byte read_byte_impl(Address address) override;

        private:
                Address apply_mirroring(Address address) const noexcept;

                std::array<Byte, real_size> ram_ {0};
        };

        class AccessibleMemory : public Memory {
        public:
                using Pieces = std::vector<Memory*>;
                explicit AccessibleMemory(Pieces pieces) noexcept;

        protected:
                bool address_is_writable_impl(Address address) const noexcept override;
                bool address_is_readable_impl(Address address) const noexcept override;
                void write_byte_impl(Address address, Byte byte) override;
                Byte read_byte_impl(Address address) override;

        private:
                Memory& find_writable_piece(Address address);
                Memory& find_readable_piece(Address address);

                template <class P>
                Memory& find_piece(P const& p)
                {
                        auto const i = std::find_if(pieces_.cbegin(),
                                                    pieces_.cend(), p);
                        assert(i != pieces_.cend());
                        return **i;
                }

                Pieces pieces_;
        };

        enum class Interrupt {
                irq,
                nmi,
                reset
        };

        static std::size_t constexpr carry_flag = 0;
        static std::size_t constexpr zero_flag = 1;
        static std::size_t constexpr interrupt_disable_flag = 2;
        static std::size_t constexpr break_flag = 4;
        static std::size_t constexpr unused_flag = 5;
        static std::size_t constexpr overflow_flag = 6;
        static std::size_t constexpr negative_flag = 7;  

        explicit CPU(AccessibleMemory::Pieces pieces);
        CPU(CPU const& other) = delete;
        CPU(CPU&& other) = default;
        CPU& operator=(CPU const& other) = delete;
        CPU& operator=(CPU&& other) = default;
        ~CPU();

        static Address constexpr stack_bottom_address = 0xFF;

        static Address interrupt_handler_address(Interrupt interrupt) noexcept;

        Address pc() const noexcept;
        Byte sp() const noexcept;
        Byte a() const noexcept;
        Byte x() const noexcept;
        Byte y() const noexcept;
        Byte p() const noexcept;
        void execute_instruction();
        void hardware_interrupt(Interrupt interrupt);

protected:
        bool address_is_readable_impl(Address address) const noexcept override;
        Byte read_byte_impl(Address address) override;

private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
};

}

