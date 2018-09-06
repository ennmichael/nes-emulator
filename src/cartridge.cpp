#include "cartridge.h"
#include <utility>
#include <cassert>

using namespace std::string_literals;

namespace Emulator {

MemoryMapperNotSupported::MemoryMapperNotSupported(Byte id) noexcept
        : runtime_error("RAM mapper " + std::to_string(id) + " not supported.")
{}

ROMImage::ROMImage(std::string const& path)
        : ROMImage(Utils::read_bytes(path))
{}

ROMImage::ROMImage(std::vector<Byte> new_data_)
        : data_(std::move(new_data_))
{
        check_data_size();
        check_header_footprint();
}

bool ROMImage::is_prg_rom(Address address) noexcept
{
        return prg_rom_lower_bank_start <= address && address <= prg_rom_upper_bank_end;
}

Byte ROMImage::num_prg_rom_banks() const noexcept
{
        return data_[4];
}

Byte ROMImage::num_chr_rom_banks() const noexcept
{
        return data_[5];
}

Byte ROMImage::mmc_id() const noexcept
{
        ByteBitset const first_half = first_control_byte() >> CHAR_BIT/2;
        ByteBitset const second_half = second_control_byte() << CHAR_BIT/2;
        return (first_half | second_half).to_ulong();
}

bool ROMImage::has_sram() const noexcept
{
        return first_control_byte().test(1);
}

bool ROMImage::has_trainer() const noexcept
{
        return first_control_byte().test(2);
}

Mirroring ROMImage::mirroring() const noexcept
{
        if (first_control_byte().test(3))
                return Mirroring::four_screen;
        else if (first_control_byte().test(0))
                return Mirroring::vertical;
        return Mirroring::horizontal;
}

bool ROMImage::has_chr_ram() const noexcept
{
        return num_chr_rom_banks() == 0;
}

ByteBitset ROMImage::first_control_byte() const noexcept
{
        return data_[6];
}

ByteBitset ROMImage::second_control_byte() const noexcept
{
        return data_[7];
}

Byte ROMImage::read_prg_rom_byte(Address address) const
{
        if (!is_prg_rom(address)) {
                throw InvalidAddress("Invalid PRG ROM address "s +
                                     Utils::format_address(address));
        }
        address = apply_mirroring(address);
        return data_[header_size + address];
}

Address ROMImage::apply_mirroring(Address address) const noexcept
{
        if (num_prg_rom_banks() == 1 && address < prg_rom_upper_bank_start)
                address += prg_rom_bank_size;
        return address;
}

void ROMImage::check_data_size() const
{
        if (data_.size() < header_size)
                throw InvalidCartridgeHeader("Cartridge header too small.");
}

void ROMImage::check_header_footprint() const
{
        if (data_[0] != 'N' ||
            data_[1] != 'E' ||
            data_[2] != 'S' ||
            data_[3] != 0x1Au) {
                throw InvalidCartridgeHeader("Invalid cartridge header footprint.");
        }
}

std::unique_ptr<Cartridge> Cartridge::make(std::string const& path)
{
        return make(ROMImage(path));
}

std::unique_ptr<Cartridge> Cartridge::make(ROMImage rom_image)
{
        if (rom_image.has_trainer())
                throw InvalidCartridge("Trainers are not supported.");

        switch (rom_image.mmc_id()) {
                case NROM::id: return std::make_unique<NROM>(std::move(rom_image));
                default:       throw MemoryMapperNotSupported(rom_image.mmc_id());
        }
}

NROM::NROM(ROMImage rom_image)
        : rom_image_(std::move(rom_image))
{
        assert(rom_image_.mmc_id() == id);

        if (rom_image_.num_prg_rom_banks() != 1 &&
            rom_image_.num_prg_rom_banks() != 2) {
                throw InvalidCartridgeHeader(
                        "NROM must have either 1 or 2 "
                        "16 KB PRG ROM banks. This one has "s +
                        std::to_string(rom_image_.num_prg_rom_banks()) +
                        "."s);
        }

        if (rom_image_.num_chr_rom_banks() != 1) {
                throw InvalidCartridgeHeader(
                        "NROM must have a single 8 KB "
                        "CHR ROM bank. This one has "s +
                        std::to_string(rom_image_.num_chr_rom_banks()) +
                        "."s);
        }

        if (rom_image_.has_sram()) {
                throw InvalidCartridgeHeader("NROM doesn't have battery-backed SRAM, "
                                             "but this cartridge does.");
        }
}

bool NROM::address_is_writable(Address address) const noexcept
{
        return is_prg_ram(address);
}

bool NROM::address_is_readable(Address address) const noexcept
{
        return is_prg_ram(address) || ROMImage::is_prg_rom(address);
}

// TODO: I don't know how I should handle CHR-ROM. Something with the PPU?

void NROM::write_byte(Address address, Byte byte)
{
        if (!address_is_writable(address)) {
                throw InvalidAddress("Can't write to NROM address "s +
                                     Utils::format_address(address));
        }
        prg_ram_[address] = byte;
}

Byte NROM::read_byte(Address address)
{
        auto const const_this = this;
        return const_this->read_byte(address);
}

Byte NROM::read_byte(Address address) const
{
        if (is_prg_ram(address))
                return prg_ram_[address];
        else if (ROMImage::is_prg_rom(address))
                return rom_image_.read_prg_rom_byte(address);
        throw InvalidAddress("Can't read NROM address "s +
                             Utils::format_address(address));
}

bool NROM::is_prg_ram(Address address) noexcept
{
        return prg_ram_start <= address && address <= prg_ram_end;
}

}

