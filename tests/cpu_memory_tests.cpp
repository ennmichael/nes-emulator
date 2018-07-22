#include "catch.hpp"
#include "../src/cpu.h"

using namespace std::string_literals;

void require_mirrored_reading_works(Emulator::CPU::RAM& ram)
{
        for (unsigned i = Emulator::CPU::RAM::start + Emulator::CPU::RAM::real_size;
             i < Emulator::CPU::RAM::end;
             ++i) {
                auto const mirrored_adress = i - Emulator::CPU::RAM::real_size;
                REQUIRE(ram.read_byte(i) == ram.read_byte(mirrored_adress));
        }
}

TEST_CASE("Internal Emulator::CPU ram works")
{
        Emulator::CPU::RAM ram;

        for (unsigned i = Emulator::CPU::RAM::start;
             i < Emulator::CPU::RAM::start + Emulator::CPU::RAM::real_size;
             ++i) {
                unsigned const value = i % Emulator::byte_max;
                ram.write_byte(i, static_cast<Emulator::Byte>(value));
        }

        SECTION("Reading works")
        {
                for (unsigned i = Emulator::CPU::RAM::start;
                     i < Emulator::CPU::RAM::start + Emulator::CPU::RAM::real_size;
                     ++i) {
                        unsigned const value = i % Emulator::byte_max;
                        REQUIRE(ram.read_byte(i) == value);
                }
        }

        SECTION("Mirrored reading works")
        {
                require_mirrored_reading_works(ram);
        }

        SECTION("Mirrored writing works")
        {
                for (unsigned i = Emulator::CPU::RAM::start + 
                                  Emulator::CPU::RAM::real_size;
                     i < Emulator::CPU::RAM::end;
                     ++i) {
                        ram.write_byte(i, static_cast<Emulator::Byte>((i % 256) + 1));
                }

                require_mirrored_reading_works(ram);
        }
}

