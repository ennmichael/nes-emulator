#pragma once

#include "utils.h"
#include "ppu.h"
#include "cartridge.h"
#include <array>

namespace Emulator {

class CPU {
public:
        class Memory {
        public:
                static std::size_t constexpr adressable_size = 0xFFFF;
                static std::size_t constexpr internal_ram_size = 0x800;
                static std::size_t constexpr ram_mirrors_size =
                        0x2000 - internal_ram_size;

                Memory(PPU& ppu, Cartridge& cartridge) noexcept;

                void write(std::size_t address, Byte byte) noexcept;
                Byte read(std::size_t address) const noexcept;

        private:
                PPU* ppu_;
                Cartridge* cartridge_;
                std::array<Byte, internal_ram_size> ram_ {0};
        };

        class Registers {
        public:

        private:

        };

        // Functions that deal with interpreting instructions, mostly
        // And also with interrupts

private:
        Memory memory_;
        Registers registers_;
};

}

