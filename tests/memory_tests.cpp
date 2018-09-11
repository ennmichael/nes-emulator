// vim: set shiftwidth=8 tabstop=8:

#include "catch.hpp"
#include "../src/cpu.h"
#include "../src/ppu.h"
#include "../src/utils.h"

using namespace std::string_literals;

namespace {

class TestMemory : public Emulator::Memory {
public:
        static unsigned constexpr size = 100;

        explicit TestMemory(Emulator::Address start)
                : start_(start)
        {}

protected:
        bool address_is_accesible(Emulator::Address address) const noexcept
        {
                address -= start_;
                return address < size;
        }

        Emulator::Address apply_mirroring(Emulator::Address address) const noexcept
        {
                return address - start_;
        }

        bool address_is_writable_impl(Emulator::Address address) const noexcept override
        {
                return address_is_accesible(address);
        }

        bool address_is_readable_impl(Emulator::Address address) const noexcept override
        {
                return address_is_accesible(address);
        }

        void write_byte_impl(Emulator::Address address, Emulator::Byte byte) override
        {
                memory_[apply_mirroring(address)] = byte;
        }

        Emulator::Byte read_byte_impl(Emulator::Address address) override
        {
                return memory_[apply_mirroring(address)];
        }

private:
        Emulator::Address start_;
        std::array<Emulator::Byte, size> memory_;
};

void require_mirrored_reading_works(Emulator::CPU::RAM& ram)
{
        for (unsigned i = Emulator::CPU::RAM::start + Emulator::CPU::RAM::real_size;
             i < Emulator::CPU::RAM::end;
             ++i) {
                auto const mirrored_adress = i - Emulator::CPU::RAM::real_size;
                REQUIRE(ram.read_byte(i) == ram.read_byte(mirrored_adress));
        }
}

}

TEST_CASE("Emulator::CPU::RAM tests")
{
        Emulator::CPU::RAM ram;

        for (unsigned i = Emulator::CPU::RAM::start;
             i < Emulator::CPU::RAM::start + Emulator::CPU::RAM::real_size;
             ++i) {
                unsigned const value = i % Emulator::byte_max;
                ram.write_byte(i, static_cast<Emulator::Byte>(value));
        }

        SECTION("Reading and writing works")
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

TEST_CASE("CPU::AccessibleMemory tests")
{
        TestMemory first_piece(0);
        TestMemory second_piece(TestMemory::size);
        Emulator::CPU::AccessibleMemory accessible_memory({&first_piece, &second_piece});
        
        for (Emulator::Address i = 0; i < TestMemory::size * 2; ++i) {
                accessible_memory.write_byte(i, static_cast<Emulator::Byte>(i));
        }

        for (Emulator::Address i = 0; i < TestMemory::size; ++i) {
                CHECK(accessible_memory.read_byte(i) == static_cast<Emulator::Byte>(i));
                CHECK(first_piece.read_byte(i) == static_cast<Emulator::Byte>(i));
                CHECK(accessible_memory.read_byte(i + TestMemory::size) == static_cast<Emulator::Byte>(i + TestMemory::size));
                CHECK(second_piece.read_byte(i + TestMemory::size) == static_cast<Emulator::Byte>(i + TestMemory::size));
        }

        for (Emulator::Address i = 0; i < TestMemory::size; ++i) {
                first_piece.write_byte(i, static_cast<Emulator::Byte>(i * 2));
                second_piece.write_byte(i + TestMemory::size,
                                        static_cast<Emulator::Byte>((i + TestMemory::size) * 2));
        }

        for (Emulator::Address i = 0; i < TestMemory::size * 2; ++i) {
                CHECK(accessible_memory.read_byte(i) == static_cast<Emulator::Byte>(i * 2));
        }
}

TEST_CASE("VRAM tests")
{
        // TODO
}

TEST_CASE("DoubleWriteRegister tests")
{
        Emulator::DoubleWriteRegister reg;
        CHECK(reg.complete());
        CHECK(reg.read_address() == 0);
        CHECK(reg.read_low_byte() == 0);
        CHECK(reg.read_high_byte() == 0);
        reg.increment(1);
        CHECK(reg.complete());
        CHECK(reg.read_address() == 1);
        CHECK(reg.read_low_byte() == 1);
        CHECK(reg.read_high_byte() == 0);
        reg.increment(257);
        CHECK(reg.complete());
        CHECK(reg.read_address() == 258);
        CHECK(reg.read_low_byte() == 2);
        CHECK(reg.read_high_byte() == 1);
        reg.write_address(1);
        CHECK(reg.complete());
        CHECK(reg.read_address() == 1);
        CHECK(reg.read_low_byte() == 1);
        CHECK(reg.read_high_byte() == 0);
        reg.write_byte(1);
        CHECK(!reg.complete());
        CHECK(reg.read_address() == 256);
        CHECK(reg.read_low_byte() == 0);
        CHECK(reg.read_high_byte() == 1);
        reg.write_byte(2);
        CHECK(reg.complete());
        CHECK(reg.read_address() == 258);
        CHECK(reg.read_low_byte() == 2);
        CHECK(reg.read_high_byte() == 1);
}

