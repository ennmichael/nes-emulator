#include "catch.hpp"
#include "../src/cpu.h"

using namespace std::string_literals;

void require_mirrored_reading_works(Emulator::CPU::RAM const& ram)
{
        for (unsigned i = Emulator::CPU::RAM::real_size;
             i < Emulator::CPU::RAM::real_size +
                 Emulator::CPU::RAM::mirrors_size;
             ++i) {
                auto const mirrored_adress = i - Emulator::CPU::RAM::real_size;
                REQUIRE(ram.read_byte(i) == ram.read_byte(mirrored_adress));
        }
}

TEST_CASE("Internal Emulator::CPU ram works")
{
        Emulator::CPU::RAM ram;

        for (unsigned i = 0;
             i < Emulator::CPU::RAM::real_size;
             ++i) {
                unsigned const value = i % Emulator::byte_max;
                ram.write_byte(i, static_cast<Emulator::Byte>(value));
        }

        SECTION("Reading works")
        {
                for (unsigned i = 0;
                     i < Emulator::CPU::RAM::real_size;
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
                for (unsigned i = Emulator::CPU::RAM::real_size;
                     i < Emulator::CPU::RAM::real_size +
                         Emulator::CPU::RAM::mirrors_size;
                     ++i) {
                        ram.write_byte(i, static_cast<Emulator::Byte>((i % 256) + 1));
                }

                require_mirrored_reading_works(ram);
        }
}

TEST_CASE("The stack works")
{
        Emulator::CPU cpu {
                .memory = std::make_unique<Emulator::CPU::RAM>(),
        };

        Emulator::Byte const low = 0x21;
        Emulator::Byte const high = 0x7F;
        unsigned const pointer = Emulator::Utils::create_address(low, high);

        SECTION("Pushing bytes works")
        {
                Emulator::Stack::push_byte(cpu, low);

                CHECK(cpu.sp == Emulator::byte_max - 1);
                CHECK(Emulator::Stack::pull_byte(cpu) == low);
                CHECK(cpu.sp == Emulator::byte_max);

                Emulator::Stack::push_byte(cpu, high);
                Emulator::Stack::push_byte(cpu, low);

                CHECK(cpu.sp == Emulator::byte_max - 2);
                CHECK(Emulator::Stack::pull_pointer(cpu) == pointer);
                CHECK(cpu.sp == Emulator::byte_max);
        }

        SECTION("Pushing pointers works")
        {
                Emulator::Stack::push_pointer(cpu, pointer);

                CHECK(cpu.sp == Emulator::byte_max - Emulator::CPU::address_size);
                CHECK(Emulator::Stack::pull_pointer(cpu) == pointer);
                CHECK(cpu.sp == Emulator::byte_max);
        
                Emulator::Byte const byte = 0x11;

                Emulator::Stack::push_pointer(cpu, pointer);
                Emulator::Stack::push_byte(cpu, byte);

                CHECK(cpu.sp ==
                      Emulator::byte_max - Emulator::CPU::address_size - 1);
                CHECK(Emulator::Stack::pull_pointer(cpu) ==
                      Emulator::Utils::create_address(byte, low));
                CHECK(cpu.sp == Emulator::byte_max - 1);
                CHECK(Emulator::Stack::pull_byte(cpu) == high);
                CHECK(cpu.sp == Emulator::byte_max);
        }
}

