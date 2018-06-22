#include "catch.hpp"
#include "../src/cartridge.h"
#include <array>
#include <string>

using namespace std::string_literals;

TEST_CASE("NEStress header loading test")
{
        Emulator::Cartridge const cartridge("../roms/NEStress.nes"s);
        auto const header = cartridge.header();

        CHECK(header.num_prg_rom_banks == 2);
        CHECK(header.num_chr_rom_banks == 1);
        CHECK(header.has_battery_backed_sram == false);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::vertical);
        CHECK(header.memory_mapper_id == Emulator::NROM::id);
        CHECK(header.has_chr_ram() == false);
}

TEST_CASE("The Legend of Zelda header loading test")
{
        Emulator::Cartridge const cartridge("../roms/The Legend of Zelda.nes"s);
        auto const header = cartridge.header();

        CHECK(header.num_prg_rom_banks == 8);
        CHECK(header.num_chr_rom_banks == 0);
        CHECK(header.has_battery_backed_sram == true);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::horizontal);
        CHECK(header.memory_mapper_id == Emulator::MMC1::id);
        CHECK(header.has_chr_ram() == true);
}

TEST_CASE("Super Mario Bros. 1 header loading test")
{
        Emulator::Cartridge const cartridge("../roms/Super Mario Bros. 1.nes"s);
        auto const header = cartridge.header();

        CHECK(header.num_prg_rom_banks == 2);
        CHECK(header.num_chr_rom_banks == 1);
        CHECK(header.has_battery_backed_sram == false);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::vertical);
        CHECK(header.memory_mapper_id == Emulator::NROM::id);
        CHECK(header.has_chr_ram() == false);
}

TEST_CASE("Super Mario Bros. 2 header loading test")
{
        Emulator::Cartridge const cartridge("../roms/Super Mario Bros. 2.nes"s);
        auto const header = cartridge.header();

        CHECK(header.num_prg_rom_banks == 8);
        CHECK(header.num_chr_rom_banks == 16);
        CHECK(header.has_battery_backed_sram == false);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::horizontal);
        CHECK(header.memory_mapper_id == Emulator::MMC3::id);
        CHECK(header.has_chr_ram() == false);
}

TEST_CASE("Super Mario Bros. 3 header loading test") 
{
        Emulator::Cartridge const cartridge("../roms/Super Mario Bros. 3.nes"s);
        auto const header = cartridge.header();

        CHECK(header.num_prg_rom_banks == 16);
        CHECK(header.num_chr_rom_banks == 16);
        CHECK(header.has_battery_backed_sram == false);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::horizontal);
        CHECK(header.memory_mapper_id == Emulator::MMC3::id);
        CHECK(header.has_chr_ram() == false);
}

TEST_CASE("Loading a cartridge with a bad footer should fail")
{
        REQUIRE_THROWS_AS(Emulator::Cartridge("../roms/NEStress bad footer.nes"s),
                          Emulator::InvalidCartridge);
}

TEST_CASE("Loading a cartridge that doesn't exist on disk should fail")
{
        REQUIRE_THROWS_AS(Emulator::Cartridge("this shouldn't exist"s),
                          Emulator::Utils::CantOpenFile);
}

