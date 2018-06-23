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

namespace Emulator {

struct CPU {
        class Memory {
        public:
                static std::size_t constexpr adressable_size = 0xFFFF;
                static std::size_t constexpr internal_ram_size = 0x800;
                static std::size_t constexpr ram_mirrors_size =
                        0x2000 - internal_ram_size;

                Memory(Cartridge& cartridge, PPU& ppu) noexcept;

                void write(std::size_t address, Byte byte) noexcept;
                Byte read(std::size_t address) const noexcept;

        private:
                Cartridge* cartridge_;
                PPU* ppu_;
                std::array<Byte, internal_ram_size> ram_ {0};
        }; 
        
        std::size_t constexpr carry_flag = 0;
        std::size_t constexpr zero_flag = 1;
        std::size_t constexpr interrupt_disable_flag = 2;
        std::size_t constexpr break_flag = 4;
        std::size_t constexpr overflow_flag = 6;
        std::size_t constexpr negative_flag = 7;  

        CPU(Cartridge& cartridge, PPU& ppu) noexcept;

        Memory memory;
        unsigned pc = 0;
        Byte sp = 0;
        Byte a = 0;
        Byte x = 0;
        Byte y = 0;
        ByteBitset p(0x34); // TODO This doesn't seem correct

        void execute(Bytes const& program);

        bool status(std::size_t flag) const noexcept;
        void status(std::size_t flag, bool value) noexcept;

        void modify_carry_flag(unsigned operation_result) noexcept;
        void modify_zero_flag(unsigned operation_result) noexcept;
        void modify_overflow_flag(unsigned operation_result) noexcept;
        void modify_negative_flag(unsigned operation_result) noexcept; 
};

using Instruction = std::variant<std::function<void(CPU&)>,
                                 std::function<void(CPU&, Byte)>,
                                 std::function<void(CPU&, Byte, Byte)>>;

Instruction translate_opcode(Byte opcode);

}

