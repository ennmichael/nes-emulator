#include "cpu.h"

namespace Emulator {

CPU::Memory::Memory(PPU& ppu, Cartridge& cartridge) noexcept
        : ppu_(&ppu)
        , cartridge_(&cartridge)
{}

void CPU::Memory::write(std::size_t address, Byte byte) noexcept
{
        ram_[address % internal_ram_size] = byte;
}

Byte CPU::Memory::read(std::size_t address) const noexcept
{
        return ram_[address % internal_ram_size];
}

}

