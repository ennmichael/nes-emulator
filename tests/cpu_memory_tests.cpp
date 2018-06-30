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

        SECTION("The stack works")
        {
                // The initial value of sp doesn't really matter for testing
                Emulator::Byte sp = Emulator::byte_max;
                Emulator::CPU::Stack stack(ram, sp);

                Emulator::Byte const low = 0x21;
                Emulator::Byte const high = 0x7F;
                unsigned const pointer = Emulator::Utils::create_address(low, high);

                SECTION("Pushing bytes works")
                {
                        stack.push_byte(low);

                        CHECK(sp == Emulator::byte_max - 1);
                        CHECK(stack.pull_byte() == low);
                        CHECK(sp == Emulator::byte_max);

                        stack.push_byte(high);
                        stack.push_byte(low);

                        CHECK(sp == Emulator::byte_max - 2);
                        CHECK(stack.pull_pointer() == pointer);
                        CHECK(sp == Emulator::byte_max);
                }

                SECTION("Pushing pointers works")
                {
                        stack.push_pointer(pointer);

                        CHECK(sp == Emulator::byte_max - Emulator::CPU::address_size);
                        CHECK(stack.pull_pointer() == pointer);
                        CHECK(sp == Emulator::byte_max);
                
                        Emulator::Byte const byte = 0x11;

                        stack.push_pointer(pointer);
                        stack.push_byte(byte);

                        CHECK(sp ==
                              Emulator::byte_max - Emulator::CPU::address_size - 1);
                        CHECK(stack.pull_pointer() ==
                              Emulator::Utils::create_address(byte, low));
                        CHECK(sp == Emulator::byte_max - 1);
                        CHECK(stack.pull_byte() == high);
                        CHECK(sp == Emulator::byte_max);
                }
        }
}

