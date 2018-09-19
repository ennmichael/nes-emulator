// vim: set shiftwidth=8 tabstop=8:

#include "catch.hpp"
#include "mem.h"
#include "../src/ppu.h"

namespace {

void check_nametable_mirroring(Emulator::VRAM& vram)
{
        for (Emulator::Address i = 0x2000; i < 0x2EFF; ++i)
                CHECK(vram.read_byte(i) == vram.read_byte(i + 0x1000));
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

TEST_CASE("VRAM palette tests")
{
        Emulator::VRAM vram(Emulator::Mirroring::horizontal);

        vram.write_byte(0x3F00, 0x69);
        for (Emulator::Address i = 0x3F00; i < 0x3F20; ++i) {
                if (i % 4 != 0)
                        vram.write_byte(i, static_cast<Emulator::Byte>(i));
        }
        for (Emulator::Address i = 0x3F00; i < 0x3F20; ++i) {
                if (i % 4 == 0)
                        CHECK(vram.read_byte(i) == 0x29);
                else
                        CHECK(vram.read_byte(i) == (static_cast<Emulator::Byte>(i) & 0x3F));
        }
        for (Emulator::Address i = 0x3F20; i < 0x4000; ++i)
                CHECK(vram.read_byte(i) == vram.read_byte(0x3F00 + (i % 0x20)));
}

TEST_CASE("DoubleRegister tests")
{
        Emulator::DoubleRegister reg;
        CHECK(reg.complete());
        CHECK(reg.read_whole() == 0);
        CHECK(reg.read_low_byte() == 0);
        CHECK(reg.read_high_byte() == 0);
        reg.increment(1);
        CHECK(reg.complete());
        CHECK(reg.read_whole() == 1);
        CHECK(reg.read_low_byte() == 1);
        CHECK(reg.read_high_byte() == 0);
        reg.increment(257);
        CHECK(reg.complete());
        CHECK(reg.read_whole() == 258);
        CHECK(reg.read_low_byte() == 2);
        CHECK(reg.read_high_byte() == 1);
        reg.write_address(1);
        CHECK(reg.complete());
        CHECK(reg.read_whole() == 1);
        CHECK(reg.read_low_byte() == 1);
        CHECK(reg.read_high_byte() == 0);
        reg.write_byte(1);
        CHECK(!reg.complete());
        CHECK(reg.read_whole() == 256);
        CHECK(reg.read_low_byte() == 0);
        CHECK(reg.read_high_byte() == 1);
        reg.write_byte(2);
        CHECK(reg.complete());
        CHECK(reg.read_whole() == 258);
        CHECK(reg.read_low_byte() == 2);
        CHECK(reg.read_high_byte() == 1);
}

TEST_CASE("PPU registers tests")
{
        TestMemory<100> test_memory(0);
        Emulator::PPU ppu(Emulator::Mirroring::horizontal, test_memory);

        SECTION("Control register tests")
        {
                ppu.write_byte(0x2000, 0x00);
                CHECK(ppu.base_name_table_address() == 0x2000);
                CHECK(ppu.vram_address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x0000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 8);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0xFD);
                CHECK(ppu.base_name_table_address() == 0x2400);
                CHECK(ppu.vram_address_increment_offset() == 32);
                CHECK(ppu.sprite_pattern_table_address() == 0x1000);
                CHECK(ppu.background_pattern_table_address() == 0x1000);
                CHECK(ppu.sprite_height() == 16);
                CHECK(ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x02);
                CHECK(ppu.base_name_table_address() == 0x2800);
                CHECK(ppu.vram_address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x0000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 8);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x03);
                CHECK(ppu.base_name_table_address() == 0x2C00);
                CHECK(ppu.vram_address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x0000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 8);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x89);
                CHECK(ppu.base_name_table_address() == 0x2400);
                CHECK(ppu.vram_address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x1000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 8);
                CHECK(ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x23);
                CHECK(ppu.base_name_table_address() == 0x2C00);
                CHECK(ppu.vram_address_increment_offset() == 1);
                CHECK(ppu.sprite_pattern_table_address() == 0x0000);
                CHECK(ppu.background_pattern_table_address() == 0x0000);
                CHECK(ppu.sprite_height() == 16);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x7E);
                CHECK(ppu.base_name_table_address() == 0x2800);
                CHECK(ppu.vram_address_increment_offset() == 32);
                CHECK(ppu.sprite_pattern_table_address() == 0x1000);
                CHECK(ppu.background_pattern_table_address() == 0x1000);
                CHECK(ppu.sprite_height() == 16);
                CHECK(!ppu.nmi_enabled());
                ppu.write_byte(0x2000, 0x11);
                CHECK(ppu.base_name_table_address() == 0x2400);
                CHECK(ppu.vram_address_increment_offset() == 1);
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

        SECTION("Status register tests")
        {
                // TODO Implement sprite #0 hit (the status register)
                // Sprite #0 hit is weird, how should I implement it?
                // Should I implement it at all? I don't emulate rendering
                // in a traditional fashion (lazyness)
        }

        SECTION("Address/data registers tests")
        {
                ppu.write_byte(0x2006, 0x20);
                ppu.write_byte(0x2006, 0x00);
                CHECK(ppu.read_vram_address_register() == 0x2000);
                ppu.write_byte(0x2007, 2);
                CHECK(ppu.read_vram_byte(0x2000) == 2);
                CHECK(ppu.read_vram_address_register() == 0x2001);
                ppu.write_byte(0x2007, 10);
                CHECK(ppu.read_vram_byte(0x2000) == 2);
                CHECK(ppu.read_vram_byte(0x2001) == 10);
                CHECK(ppu.read_vram_address_register() == 0x2002);
                ppu.write_byte(0x2007, 25);
                CHECK(ppu.read_vram_byte(0x2000) == 2);
                CHECK(ppu.read_vram_byte(0x2001) == 10);
                CHECK(ppu.read_vram_byte(0x2002) == 25);
                CHECK(ppu.read_vram_address_register() == 0x2003);

                ppu.write_byte(0x2006, 0x20);
                ppu.write_byte(0x2006, 0x00);
                CHECK(ppu.read_vram_address_register() == 0x2000);
                ppu.read_byte(0x2007);
                CHECK(ppu.read_vram_address_register() == 0x2001);
                CHECK(ppu.read_byte(0x2007) == 2);
                CHECK(ppu.read_vram_address_register() == 0x2002);
                CHECK(ppu.read_byte(0x2007) == 10);
                CHECK(ppu.read_vram_address_register() == 0x2003);
                CHECK(ppu.read_byte(0x2007) == 25);
                CHECK(ppu.read_vram_address_register() == 0x2004);

                ppu.write_byte(0x2006, 0x21);
                ppu.write_byte(0x2006, 0x10);
                ppu.write_byte(0x2000, 0x04);
                CHECK(ppu.vram_address_increment_offset() == 0x20);
                CHECK(ppu.read_vram_address_register() == 0x2110);
                ppu.write_byte(0x2007, 2);
                CHECK(ppu.read_vram_byte(0x2000) == 2);
                CHECK(ppu.read_vram_address_register() == 0x2130);
                ppu.write_byte(0x2007, 10);
                CHECK(ppu.read_vram_byte(0x2000) == 2);
                CHECK(ppu.read_vram_byte(0x2130) == 10);
                CHECK(ppu.read_vram_address_register() == 0x2150);
                ppu.write_byte(0x2007, 25);
                CHECK(ppu.read_vram_byte(0x2000) == 2);
                CHECK(ppu.read_vram_byte(0x2130) == 10);
                CHECK(ppu.read_vram_byte(0x2150) == 25);
                CHECK(ppu.read_vram_address_register() == 0x2170);

                ppu.write_byte(0x2006, 0x21);
                ppu.write_byte(0x2006, 0x10);
                CHECK(ppu.read_vram_address_register() == 0x2110);
                ppu.read_byte(0x2007);
                CHECK(ppu.read_vram_address_register() == 0x2130);
                CHECK(ppu.read_byte(0x2007) == 2);
                CHECK(ppu.read_vram_address_register() == 0x2150);
                CHECK(ppu.read_byte(0x2007) == 10);
                CHECK(ppu.read_vram_address_register() == 0x2170);
                CHECK(ppu.read_byte(0x2007) == 25);
                CHECK(ppu.read_vram_address_register() == 0x2190);
        }

        SECTION("OAM address/data registers tests")
        {
                // TODO
        }
        
        SECTION("OAM DMA tests")
        {
                // TODO
        }
}

TEST_CASE("PPU screen tests")
{

}

TEST_CASE("PPU background painting tests")
{
        // TODO
}

TEST_CASE("PPU sprite paining tests")
{
        // TODO
}

TEST_CASE("PPU screen painting tests")
{
        // TODO
}

