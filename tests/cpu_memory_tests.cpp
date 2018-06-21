#include "catch.hpp"
#include "../../src/cpu.h"

using namespace std::string_literals;

void require_mirrored_reading_works(Emulator::CPU::Memory const& memory)
{
        for (std::size_t i = Emulator::CPU::Memory::internal_ram_size;
             i < Emulator::CPU::Memory::internal_ram_size +
                 Emulator::CPU::Memory::ram_mirrors_size;
             ++i) {
                auto const mirrored_adress = i - Emulator::CPU::Memory::ram_mirrors_size;
                REQUIRE(memory.read(i) == memory.read(mirrored_adress));
        }
}

TEST_CASE("Internal Emulator::CPU memory tests")
{
        Emulator::PPU ppu;
        Emulator::Cartridge cartridge("../roms/NEStress.nes"s);
        Emulator::CPU::Memory memory(ppu, cartridge);

        for (std::size_t i = 0;
             i < Emulator::CPU::Memory::internal_ram_size;
             ++i) {
                memory.write(i, static_cast<Emulator::Byte>(i % Emulator::byte_max));
        }

        SECTION("Reading works")
        {
                for (std::size_t i = 0;
                     i < Emulator::CPU::Memory::internal_ram_size;
                     ++i) {
                        auto const value =
                                static_cast<Emulator::Byte>(i % Emulator::byte_max);
                        REQUIRE(memory.read(i) == value);
                }
        }

        SECTION("Mirrored reading works")
        {
                require_mirrored_reading_works(memory);
        }

        SECTION("Mirrored writing works")
        {
                for (std::size_t i = Emulator::CPU::Memory::internal_ram_size;
                     i < Emulator::CPU::Memory::internal_ram_size +
                         Emulator::CPU::Memory::ram_mirrors_size;
                     ++i) {
                        memory.write(i, static_cast<Emulator::Byte>((i % 256) + 1));
                }

                require_mirrored_reading_works(memory);
        }
}

