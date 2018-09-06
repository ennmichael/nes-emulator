#include "catch.hpp"
#include "../src/cartridge.h"
#include <array>
#include <string>

using namespace std::string_literals;

TEST_CASE("NEStress header test")
{
        Emulator::ROMImage rom_image("../roms/NEStress.nes"s);

        CHECK(rom_image.num_prg_rom_banks() == 2);
        CHECK(rom_image.num_chr_rom_banks() == 1);
        CHECK(rom_image.has_sram() == false);
        CHECK(rom_image.has_trainer() == false);
        CHECK(rom_image.mirroring() == Emulator::Mirroring::vertical);
        CHECK(rom_image.mmc_id() == Emulator::NROM::id);
        CHECK(rom_image.has_chr_ram() == false);
}

TEST_CASE("The Legend of Zelda header test")
{
        Emulator::ROMImage rom_image("../roms/The Legend of Zelda.nes"s);

        CHECK(rom_image.num_prg_rom_banks() == 8);
        CHECK(rom_image.num_chr_rom_banks() == 0);
        CHECK(rom_image.has_sram() == true);
        CHECK(rom_image.has_trainer() == false);
        CHECK(rom_image.mirroring() == Emulator::Mirroring::horizontal);
        CHECK(rom_image.mmc_id() == Emulator::MMC1::id);
        CHECK(rom_image.has_chr_ram() == true);
}

TEST_CASE("Super Mario Bros. 1 header test")
{
        Emulator::ROMImage rom_image("../roms/Super Mario Bros. 1.nes"s);

        CHECK(rom_image.num_prg_rom_banks() == 2);
        CHECK(rom_image.num_chr_rom_banks() == 1);
        CHECK(rom_image.has_sram() == false);
        CHECK(rom_image.has_trainer() == false);
        CHECK(rom_image.mirroring() == Emulator::Mirroring::vertical);
        CHECK(rom_image.mmc_id() == Emulator::NROM::id);
        CHECK(rom_image.has_chr_ram() == false);
}

TEST_CASE("Super Mario Bros. 2 header test")
{
        Emulator::ROMImage rom_image("../roms/Super Mario Bros. 2.nes"s);

        CHECK(rom_image.num_prg_rom_banks() == 8);
        CHECK(rom_image.num_chr_rom_banks() == 16);
        CHECK(rom_image.has_sram() == false);
        CHECK(rom_image.has_trainer() == false);
        CHECK(rom_image.mirroring() == Emulator::Mirroring::horizontal);
        CHECK(rom_image.mmc_id() == Emulator::MMC3::id);
        CHECK(rom_image.has_chr_ram() == false);
}

TEST_CASE("Super Mario Bros. 3 header test") 
{
        Emulator::ROMImage rom_image("../roms/Super Mario Bros. 3.nes"s);

        CHECK(rom_image.num_prg_rom_banks() == 16);
        CHECK(rom_image.num_chr_rom_banks() == 16);
        CHECK(rom_image.has_sram() == false);
        CHECK(rom_image.has_trainer() == false);
        CHECK(rom_image.mirroring() == Emulator::Mirroring::horizontal);
        CHECK(rom_image.mmc_id() == Emulator::MMC3::id);
        CHECK(rom_image.has_chr_ram() == false);
}

TEST_CASE("Loading a cartridge with a bad footprint should fail")
{
        REQUIRE_THROWS_AS(Emulator::ROMImage("../roms/NEStress bad footprint.nes"s),
                          Emulator::InvalidCartridgeHeader);
}

TEST_CASE("Loading a cartridge that doesn't exist on disk should fail")
{
        REQUIRE_THROWS_AS(Emulator::ROMImage("this shouldn't exist"s),
                          Emulator::Utils::CantOpenFile);
}

