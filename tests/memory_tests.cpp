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

void check_ram_mirroring(Emulator::CPU::RAM& ram)
{
        for (unsigned i = Emulator::CPU::RAM::start + Emulator::CPU::RAM::real_size;
             i < Emulator::CPU::RAM::end;
             ++i) {
                auto const mirrored_adress = i - Emulator::CPU::RAM::real_size;
                CHECK(ram.read_byte(i) == ram.read_byte(mirrored_adress));
        }
}

void check_nametable_mirroring(Emulator::VRAM& vram)
{
        for (Emulator::Address i = 0x2000; i < 0x2EFF; ++i)
                CHECK(vram.read_byte(i) == vram.read_byte(i + 0x1000));
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
                check_ram_mirroring(ram);
        }

        SECTION("Mirrored writing works")
        {
                for (unsigned i = Emulator::CPU::RAM::start + 
                                  Emulator::CPU::RAM::real_size;
                     i < Emulator::CPU::RAM::end;
                     ++i) {
                        ram.write_byte(i, static_cast<Emulator::Byte>((i % 256) + 1));
                }

                check_ram_mirroring(ram);
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

TEST_CASE("VRAM pattern tables tests")
{
        Emulator::VRAM vram(Emulator::Mirroring::horizontal);
        for (Emulator::Address i = 0; i < 0x2000; ++i)
                vram.write_byte(i, static_cast<Emulator::Byte>(i));
        for (Emulator::Address i = 0; i < 0x2000; ++i)
                CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i));
}

TEST_CASE("VRAM horizontal mirroring nametables tests")
{
        Emulator::VRAM vram(Emulator::Mirroring::horizontal);
        
        SECTION("Writing to first nametable and reading from second")
        {
                for (Emulator::Address i = 0x2000; i < 0x2400; ++i)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i));
                for (Emulator::Address i = 0x2400; i < 0x2800; ++i)
                        CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i));
                check_nametable_mirroring(vram);
        }

        SECTION("Writing to second nametable and reading from first")
        {
                for (Emulator::Address i = 0x2400; i < 0x2800; ++i)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i + 1));
                for (Emulator::Address i = 0x2000; i < 0x2400; ++i)
                        CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i + 1));
                check_nametable_mirroring(vram);
        }

        SECTION("Writing to third nametable and reading from fourth")
        {
                for (Emulator::Address i = 0x2800; i < 0x2C00; ++i)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i + 2));
                for (Emulator::Address i = 0x2C00; i < 0x3000; ++i)
                        CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i + 2));
                check_nametable_mirroring(vram);
        }

        SECTION("Writing to fourth nametable and reading from third")
        {
                for (Emulator::Address i = 0x2800; i < 0x2C00; ++i)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i + 3));
                for (Emulator::Address i = 0x2C00; i < 0x3000; ++i)
                        CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i + 3));
                check_nametable_mirroring(vram);
        }
}

TEST_CASE("VRAM vertical mirroring nametables tests")
{
        Emulator::VRAM vram(Emulator::Mirroring::vertical);

        SECTION("Writing to first nametable and reading from third")
        {
                for (Emulator::Address i = 0x2800; i < 0x2C00; ++i)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i));
                for (Emulator::Address i = 0x2000; i < 0x2400; ++i)
                        CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i));
                check_nametable_mirroring(vram);
        }

        SECTION("Writing to third nametable and reading from first")
        {
                for (Emulator::Address i = 0x2800; i < 0x2C00; ++i)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i + 1));
                for (Emulator::Address i = 0x2000; i < 0x2400; ++i)
                        CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i + 1));
                check_nametable_mirroring(vram);
        }

        SECTION("Writing to second nametable and reading from fourth")
        {
                for (Emulator::Address i = 0x2400; i < 0x2800; ++i)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i + 2));
                for (Emulator::Address i = 0x2C00; i < 0x3000; ++i)
                        CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i + 2));
                check_nametable_mirroring(vram);
        }

        SECTION("Writing to fourth nametable and reading from second")
        {
                for (Emulator::Address i = 0x2C00; i < 0x3000; ++i)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i + 2));
                for (Emulator::Address i = 0x2400; i < 0x2800; ++i)
                        CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i + 2));
                check_nametable_mirroring(vram);
        }
}

TEST_CASE("VRAM four-screen mirroring nametables tests")
{
        Emulator::VRAM vram(Emulator::Mirroring::four_screen);
        for (Emulator::Address i = 0; i < 0x3000; ++i)
                vram.write_byte(i, static_cast<Emulator::Byte>(i % 254));
        for (Emulator::Address i = 0; i < 0x3000; ++i)
                CHECK(vram.read_byte(i) == static_cast<Emulator::Byte>(i % 254));
        check_nametable_mirroring(vram);
}

/*
TEST_CASE("VRAM palette tests")
{
        Emulator::VRAM vram(Emulator::Mirroring::horizontal);

        vram.write_byte(0x3F00, 0x69);
        for (Emulator::Address i = 0; i < 0x3F20; ++i) {
                if (i % 4 != 0)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i));
        }
        for (Emulator::Address i = 0; i < 0x3F20; ++i) {
                if (i % 4 == 0)
                        CHECK(vram.read_byte(i) == 0x68);
                else
                        CHECK(vram.read_byte(i) == (static_cast<Emulator::Byte>(i) & 0x3F));
        }
}
*/

TEST_CASE("DoubleRegister tests")
{
        Emulator::DoubleRegister reg;
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

TEST_CASE("PPU registers tests")
{
        TestMemory test_memory(0);
        Emulator::PPU ppu(Emulator::Mirroring::horizontal, test_memory);

        SECTION("Control register tests")
        {
                ppu.write_byte(0x2000, 0x00);
                CHECK(ppu.base_name_table_address() == 0x2000);
                CHECK(ppu.address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x0000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 8);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0xFD);
                CHECK(ppu.base_name_table_address() == 0x2400);
                CHECK(ppu.address_increment_offset() == 32);
                CHECK(ppu.sprite_pattern_table_address() == 0x1000);
                CHECK(ppu.background_pattern_table_address() == 0x1000);
                CHECK(ppu.sprite_height() == 16);
                CHECK(ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x02);
                CHECK(ppu.base_name_table_address() == 0x2800);
                CHECK(ppu.address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x0000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 8);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x03);
                CHECK(ppu.base_name_table_address() == 0x2C00);
                CHECK(ppu.address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x0000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 8);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x89);
                CHECK(ppu.base_name_table_address() == 0x2400);
                CHECK(ppu.address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x1000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 8);
                CHECK(ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x23);
                CHECK(ppu.base_name_table_address() == 0x2C00);
                CHECK(ppu.address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x0000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 16);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x7E);
                CHECK(ppu.base_name_table_address() == 0x2800);
                CHECK(ppu.address_increment_offset() == 32);
                CHECK(ppu.sprite_pattern_table_address() == 0x1000);
                CHECK(ppu.background_pattern_table_address() == 0x1000);
                CHECK(ppu.sprite_height() == 16);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x11);
                CHECK(ppu.base_name_table_address() == 0x2400);
                CHECK(ppu.address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x0000);
                CHECK(ppu.background_pattern_table_address() == 0x1000);
                CHECK(ppu.sprite_height() == 8);
                CHECK(!ppu.nmi_enabled());
        }

        SECTION("Mask register tests")
        {
                ppu.write_byte(0x2001, 0x00);
                CHECK(!ppu.greyscale());
                CHECK(!ppu.show_leftmost_background());
                CHECK(!ppu.show_leftmost_sprites());
                CHECK(!ppu.show_background());
                CHECK(!ppu.show_sprites());
                ppu.write_byte(0x2001, 0xFF);
                CHECK(ppu.greyscale());
                CHECK(ppu.show_leftmost_background());
                CHECK(ppu.show_leftmost_sprites());
                CHECK(ppu.show_background());
                CHECK(ppu.show_sprites());
                ppu.write_byte(0x2001, 0x57);
                CHECK(ppu.greyscale());
                CHECK(ppu.show_leftmost_background());
                CHECK(ppu.show_leftmost_sprites());
                CHECK(!ppu.show_background());
                CHECK(ppu.show_sprites());
                ppu.write_byte(0x2001, 0x6E);
                CHECK(!ppu.greyscale());
                CHECK(ppu.show_leftmost_background());
                CHECK(ppu.show_leftmost_sprites());
                CHECK(ppu.show_background());
                CHECK(!ppu.show_sprites());
                ppu.write_byte(0x2001, 0x6E);
                CHECK(!ppu.greyscale());
                CHECK(ppu.show_leftmost_background());
                CHECK(ppu.show_leftmost_sprites());
                CHECK(ppu.show_background());
                CHECK(!ppu.show_sprites());
                ppu.write_byte(0x2001, 0x84);
                CHECK(!ppu.greyscale());
                CHECK(!ppu.show_leftmost_background());
                CHECK(ppu.show_leftmost_sprites());
                CHECK(!ppu.show_background());
                CHECK(!ppu.show_sprites());
                ppu.write_byte(0x2001, 0x4B);
                CHECK(ppu.greyscale());
                CHECK(ppu.show_leftmost_background());
                CHECK(!ppu.show_leftmost_sprites());
                CHECK(ppu.show_background());
                CHECK(!ppu.show_sprites());
                ppu.write_byte(0x2001, 0x54);
                CHECK(!ppu.greyscale());
                CHECK(!ppu.show_leftmost_background());
                CHECK(ppu.show_leftmost_sprites());
                CHECK(!ppu.show_background());
                CHECK(ppu.show_sprites());
        }

        SECTION("Status register")
        {
                // TODO
        }
}

