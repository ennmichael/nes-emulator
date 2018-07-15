#include "catch.hpp"
#include "../src/cartridge.h"
#include <array>
#include <string>

using namespace std::string_literals;

TEST_CASE("NEStress header loading test")
{
        auto const header =
                Emulator::Cartridge::Header::parse("../roms/NEStress.nes"s);

        CHECK(header.num_prg_rom_banks == 2);
        CHECK(header.num_chr_rom_banks == 1);
        CHECK(header.has_battery_backed_sram == false);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::vertical);
        CHECK(header.mmc_id == Emulator::NROM::id);
        CHECK(header.has_chr_ram() == false);
}

TEST_CASE("The Legend of Zelda header loading test")
{
        auto const header =
                Emulator::Cartridge::Header::parse("../roms/The Legend of Zelda.nes"s);

        CHECK(header.num_prg_rom_banks == 8);
        CHECK(header.num_chr_rom_banks == 0);
        CHECK(header.has_battery_backed_sram == true);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::horizontal);
        CHECK(header.mmc_id == Emulator::MMC1::id);
        CHECK(header.has_chr_ram() == true);
}

TEST_CASE("Super Mario Bros. 1 header loading test")
{
        auto const header =
                Emulator::Cartridge::Header::parse("../roms/Super Mario Bros. 1.nes"s);

        CHECK(header.num_prg_rom_banks == 2);
        CHECK(header.num_chr_rom_banks == 1);
        CHECK(header.has_battery_backed_sram == false);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::vertical);
        CHECK(header.mmc_id == Emulator::NROM::id);
        CHECK(header.has_chr_ram() == false);
}

TEST_CASE("Super Mario Bros. 2 header loading test")
{
        auto const header =
                Emulator::Cartridge::Header::parse("../roms/Super Mario Bros. 2.nes"s);

        CHECK(header.num_prg_rom_banks == 8);
        CHECK(header.num_chr_rom_banks == 16);
        CHECK(header.has_battery_backed_sram == false);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::horizontal);
        CHECK(header.mmc_id == Emulator::MMC3::id);
        CHECK(header.has_chr_ram() == false);
}

TEST_CASE("Super Mario Bros. 3 header loading test") 
{
        auto const header =
                Emulator::Cartridge::Header::parse("../roms/Super Mario Bros. 3.nes"s);

        CHECK(header.num_prg_rom_banks == 16);
        CHECK(header.num_chr_rom_banks == 16);
        CHECK(header.has_battery_backed_sram == false);
        CHECK(header.has_trainer == false);
        CHECK(header.mirroring == Emulator::Mirroring::horizontal);
        CHECK(header.mmc_id == Emulator::MMC3::id);
        CHECK(header.has_chr_ram() == false);
}

TEST_CASE("Loading a cartridge with a bad footprint should fail")
{
        REQUIRE_THROWS_AS(Emulator::Cartridge::Header::parse(
                                  "../roms/NEStress bad footprint.nes"s),
                          Emulator::InvalidCartridgeHeader);
}

TEST_CASE("Loading a cartridge that doesn't exist on disk should fail")
{
        REQUIRE_THROWS_AS(Emulator::Cartridge::Header::parse("this shouldn't exist"s),
                          Emulator::Utils::CantOpenFile);
}

