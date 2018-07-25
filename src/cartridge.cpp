#include "cartridge.h"
#include <utility>
#include <cassert>

using namespace std::string_literals;

namespace Emulator {

MemoryMapperNotSupported::MemoryMapperNotSupported(Byte id) noexcept
        : runtime_error("RAM mapper " + std::to_string(id) + " not supported.")
{}

NESFile::NESFile(std::string const& path)
        : NESFile(Utils::read_bytes(path))
{}

NESFile::NESFile(ByteVector new_data)
        : data(std::move(new_data))
{
        check_data_size();
        check_header_footprint();
}

Byte NESFile::num_prg_rom_banks() const noexcept
{
        return data[4];
}

Byte NESFile::num_chr_rom_banks() const noexcept
{
        return data[5];
}

Byte NESFile::mmc_id() const noexcept
{
        ByteBitset const first_half = first_control_byte() >> CHAR_BIT/2;
        ByteBitset const second_half = second_control_byte() << CHAR_BIT/2;
        return (first_half | second_half).to_ulong();
}

bool NESFile::has_sram() const noexcept
{
        return first_control_byte().test(1);
}

bool NESFile::has_trainer() const noexcept
{
        return first_control_byte().test(2);
}

Mirroring NESFile::mirroring() const noexcept
{
        if (first_control_byte().test(3))
                return Mirroring::four_screen;
        else if (first_control_byte().test(0))
                return Mirroring::vertical;
        return Mirroring::horizontal;
}

bool NESFile::has_chr_ram() const noexcept
{
        return num_chr_rom_banks() == 0;
}

ByteBitset NESFile::first_control_byte() const noexcept
{
        return data[6];
}

ByteBitset NESFile::second_control_byte() const noexcept
{
        return data[7];
}

void NESFile::check_data_size() const
{
        if (data.size() < header_size)
                throw InvalidCartridgeHeader("Cartridge header too small.");
}

void NESFile::check_header_footprint() const
{
        if (data[0] != 'N' ||
            data[1] != 'E' ||
            data[2] != 'S' ||
            data[3] != 0x1Au)
                throw InvalidCartridgeHeader("Invalid cartridge header footprint.");
}

UniqueCartridge Cartridge::make(std::string const& path)
{
        return make(NESFile(path));
}

UniqueCartridge Cartridge::make(NESFile nes_file)
{
        if (nes_file.has_trainer())
                throw InvalidCartridge("Trainers are not supported.");

        switch (nes_file.mmc_id()) {
                case NROM::id: return std::make_unique<NROM>(std::move(nes_file));
                default:       throw MemoryMapperNotSupported(nes_file.mmc_id());
        }
}

NROM::NROM(NESFile nes_file)
        : nes_file_(std::move(nes_file))
{
        assert(nes_file_.mmc_id() == id);

        if (nes_file_.num_prg_rom_banks() != 1 &&
            nes_file_.num_prg_rom_banks() != 2) {
                throw InvalidCartridgeHeader(
                        "NROM must have either 1 or 2 "
                        "16 KB PRG ROM banks. This one has "s +
                        std::to_string(nes_file_.num_prg_rom_banks()) +
                        "."s);
        }

        if (nes_file_.num_chr_rom_banks() != 1) {
                throw InvalidCartridgeHeader(
                        "NROM must have a single 8 KB "
                        "CHR ROM bank. This one has "s +
                        std::to_string(nes_file_.num_chr_rom_banks()) +
                        "."s);
        }

        if (nes_file_.has_sram()) {
                throw InvalidCartridgeHeader("NROM doesn't have battery-backed SRAM, "
                                             "but this cartridge does.");
        }
}

bool NROM::address_is_writable(unsigned address) const noexcept
{
        return prg_ram_start <= address && address < prg_ram_end;
}

bool NROM::address_is_readable(unsigned address) const noexcept
{
        return prg_rom_lower_bank_start <= address && 
               address < prg_rom_upper_bank_end;
}

// TODO: I don't know how I should handle CHR-ROM. Something with the PPU?

void NROM::write_byte(unsigned address, Byte byte)
{
        if (!address_is_writable(address)) {
                throw InvalidAddress("Can't write to NROM address "s +
                                     Utils::format_address(address));
        }

        prg_ram_[address] = byte;
}

Byte NROM::read_byte(unsigned address)
{
        NROM const& self = *this;
        return self.read_byte(address);
}

Byte NROM::read_byte(unsigned address) const
{
        if (!address_is_readable(address)) {
                throw InvalidAddress("Can't read NROM address "s +
                                     Utils::format_address(address));
        }

        if (address_is_writable(address))
                return prg_ram_[address];

        if (nes_file_.num_prg_rom_banks() == 1 && address < prg_rom_upper_bank_start)
                address += prg_rom_bank_size;

        address += NESFile::prg_rom_start;
        return nes_file_.data[address];
}

}

