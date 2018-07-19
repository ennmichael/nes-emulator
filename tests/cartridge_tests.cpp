#include "catch.hpp"
#include "../src/cartridge.h"
#include <array>
#include <string>

using namespace std::string_literals;

TEST_CASE("NEStress file loading test")
{
        Emulator::NESFile file("../roms/NEStress.nes"s);

        CHECK(file.num_prg_rom_banks() == 2);
        CHECK(file.num_chr_rom_banks() == 1);
        CHECK(file.has_sram() == false);
        CHECK(file.has_trainer() == false);
        CHECK(file.mirroring() == Emulator::Mirroring::vertical);
        CHECK(file.mmc_id() == Emulator::NROM::id);
        CHECK(file.has_chr_ram() == false);
}

TEST_CASE("The Legend of Zelda file loading test")
{
        Emulator::NESFile file("../roms/The Legend of Zelda.nes"s);

        CHECK(file.num_prg_rom_banks() == 8);
        CHECK(file.num_chr_rom_banks() == 0);
        CHECK(file.has_sram() == true);
        CHECK(file.has_trainer() == false);
        CHECK(file.mirroring() == Emulator::Mirroring::horizontal);
        CHECK(file.mmc_id() == Emulator::MMC1::id);
        CHECK(file.has_chr_ram() == true);
}

TEST_CASE("Super Mario Bros. 1 file loading test")
{
        Emulator::NESFile file("../roms/Super Mario Bros. 1.nes"s);

        CHECK(file.num_prg_rom_banks() == 2);
        CHECK(file.num_chr_rom_banks() == 1);
        CHECK(file.has_sram() == false);
        CHECK(file.has_trainer() == false);
        CHECK(file.mirroring() == Emulator::Mirroring::vertical);
        CHECK(file.mmc_id() == Emulator::NROM::id);
        CHECK(file.has_chr_ram() == false);
}

TEST_CASE("Super Mario Bros. 2 file loading test")
{
        Emulator::NESFile file("../roms/Super Mario Bros. 2.nes"s);

        CHECK(file.num_prg_rom_banks() == 8);
        CHECK(file.num_chr_rom_banks() == 16);
        CHECK(file.has_sram() == false);
        CHECK(file.has_trainer() == false);
        CHECK(file.mirroring() == Emulator::Mirroring::horizontal);
        CHECK(file.mmc_id() == Emulator::MMC3::id);
        CHECK(file.has_chr_ram() == false);
}

TEST_CASE("Super Mario Bros. 3 file loading test") 
{
        Emulator::NESFile file("../roms/Super Mario Bros. 3.nes"s);

        CHECK(file.num_prg_rom_banks() == 16);
        CHECK(file.num_chr_rom_banks() == 16);
        CHECK(file.has_sram() == false);
        CHECK(file.has_trainer() == false);
        CHECK(file.mirroring() == Emulator::Mirroring::horizontal);
        CHECK(file.mmc_id() == Emulator::MMC3::id);
        CHECK(file.has_chr_ram() == false);
}

TEST_CASE("Loading a cartridge with a bad footprint should fail")
{
        REQUIRE_THROWS_AS(Emulator::NESFile("../roms/NEStress bad footprint.nes"s),
                          Emulator::InvalidCartridgeHeader);
}

TEST_CASE("Loading a cartridge that doesn't exist on disk should fail")
{
        REQUIRE_THROWS_AS(Emulator::NESFile("this shouldn't exist"s),
                          Emulator::Utils::CantOpenFile);
}

