#include "cartridge.h"
#include <utility>
#include <cassert>

using namespace std::string_literals;

namespace Emulator {

MemoryMapperNotSupported::MemoryMapperNotSupported(Byte id) noexcept
        : runtime_error("RAM mapper " + std::to_string(id) + " not supported.")
{}

bool Cartridge::Header::has_chr_ram() const noexcept
{
        return num_chr_rom_banks == 0;
}

auto Cartridge::Header::parse(std::string const& path) -> Header
{
        return parse(Utils::read_bytes(path));
}

auto Cartridge::Header::parse(Bytes const& data) -> Header
{
        if (data.size() < size)
                throw InvalidCartridgeHeader("Cartridge header too small.");

        if (data[0] != 'N' ||
            data[1] != 'E' ||
            data[2] != 'S' ||
            data[3] != 0x1Au)
                throw InvalidCartridgeHeader("Invalid cartridge header footprint.");

        ByteBitset const first_control_byte(data[6]);
        ByteBitset const second_control_byte(data[7]);

        return Header {
                .num_prg_rom_banks = data[4],
                .num_chr_rom_banks = data[5],
                .mmc_id = parse_mmc_id(first_control_byte, second_control_byte),
                .has_battery_backed_sram = first_control_byte.test(1),
                .has_trainer = first_control_byte.test(2),
                .mirroring = parse_mirroring(first_control_byte)
        };
}

Mirroring Cartridge::Header::parse_mirroring(ByteBitset first_control_byte) noexcept
{
        if (first_control_byte.test(3))
                return Mirroring::four_screen;
        else if (first_control_byte.test(0))
                return Mirroring::vertical;
        return Mirroring::horizontal;
}

Byte Cartridge::Header::parse_mmc_id(ByteBitset first_control_byte,
                                     ByteBitset second_control_byte) noexcept
{
        first_control_byte >>= CHAR_BIT/2;
        second_control_byte <<= CHAR_BIT/2;
        ByteBitset const result = first_control_byte | second_control_byte;
        return Utils::to_byte(result);
}

UniqueCartridge Cartridge::make(std::string const& path)
{
        return make(Utils::read_bytes(path));
}

UniqueCartridge Cartridge::make(Bytes data)
{
        auto const header = Header::parse(data);

        if (header.has_trainer)
                throw InvalidCartridge("Trainers are not supported."s);

        switch (header.mmc_id) {
                case NROM::id: return std::make_unique<NROM>(header, std::move(data));
                default:       throw MemoryMapperNotSupported(header.mmc_id);
        }
}

NROM::NROM(Header header, Bytes data)
        : header_(header)
        , data_(data)
{
        assert(header_.mmc_id == id);

        if (header_.num_prg_rom_banks != 1 && header_.num_prg_rom_banks != 2) {
                throw InvalidCartridgeHeader(
                        "NROM must have either 1 or 2 "
                        "16 KB PRG ROM banks. This one has "s +
                        std::to_string(header_.num_prg_rom_banks) +
                        "."s);
        }

        if (header_.num_chr_rom_banks != 1) {
                throw InvalidCartridgeHeader(
                        "NROM must have a single 8 KB "
                        "CHR ROM bank. This one has "s +
                        std::to_string(header_.num_chr_rom_banks) +
                        "."s);
        }

        if (header_.has_battery_backed_sram) {
                throw InvalidCartridgeHeader("NROM doesn't have battery-backed SRAM, "
                                             "but this cartridge does.");
        }
}

bool Cartridge::address_is_writable(unsigned address) const noexcept
{
        return prg_ram_start <= address && address < prg_ram_end;
}

bool Cartridge::address_is_readable(unsigned address) const noexcept
{
        return prg_rom_lower_bank_start <= address && 
               address < prg_rom_upper_bank_end;
}

// TODO: I don't know how I should handle CHR-ROM. Something with the PPU?

void NROM::do_write_byte(unsigned address, Byte byte)
{
        prg_ram_[address] = byte;
}

Byte NROM::do_read_byte(unsigned address) const
{
        if (address_is_writable(address))
                return prg_ram_[address];

        if (header_.num_prg_rom_banks == 1 && address < prg_rom_upper_bank_start)
                address += prg_rom_bank_size;

        address += nes_file_prg_rom_start;
        return data_[address];
}

}

