// vim: set shiftwidth=8 tabstop=8:

#include "cartridge.h"
#include <utility>
#include <cassert>

using namespace std::string_literals;

namespace Emulator {

MemoryMapperNotSupported::MemoryMapperNotSupported(Byte id) noexcept
        : runtime_error("RAM mapper " + std::to_string(id) + " not supported.")
{}

Cartridge::Cartridge(std::string const& path)
        : Cartridge(read_bytes(path))
{}

Cartridge::Cartridge(std::vector<Byte> data)
        : data_(std::move(data))
{
        check_data_size();
        check_header_footprint();
}

bool Cartridge::is_prg_rom(Address address) noexcept
{
        return prg_rom_lower_bank_start <= address && address <= prg_rom_upper_bank_end;
}

Byte Cartridge::num_prg_rom_banks() const noexcept
{
        return data_[4];
}

Byte Cartridge::num_chr_rom_banks() const noexcept
{
        return data_[5];
}

Byte Cartridge::mmc_id() const noexcept
{
        ByteBitset const first_half = first_control_byte() >> CHAR_BIT/2;
        ByteBitset const second_half = second_control_byte() << CHAR_BIT/2;
        return (first_half | second_half).to_ulong();
}

bool Cartridge::has_sram() const noexcept
{
        return first_control_byte().test(1);
}

bool Cartridge::has_trainer() const noexcept
{
        return first_control_byte().test(2);
}

Mirroring Cartridge::mirroring() const noexcept
{
        if (first_control_byte().test(3))
                return Mirroring::four_screen;
        else if (first_control_byte().test(0))
                return Mirroring::vertical;
        return Mirroring::horizontal;
}

bool Cartridge::has_chr_ram() const noexcept
{
        return num_chr_rom_banks() == 0;
}

ByteBitset Cartridge::first_control_byte() const noexcept
{
        return data_[6];
}

ByteBitset Cartridge::second_control_byte() const noexcept
{
        return data_[7];
}

Byte Cartridge::read_prg_rom_byte(Address address) const
{
        if (!is_prg_rom(address))
                throw InvalidRead(address);
        address = apply_mirroring(address);
        return data_[header_size + address];
}

Address Cartridge::apply_mirroring(Address address) const noexcept
{
        if (num_prg_rom_banks() == 1 && address < prg_rom_upper_bank_start)
                address += prg_rom_bank_size;
        return address;
}

void Cartridge::check_data_size() const
{
        if (data_.size() < header_size)
                throw InvalidCartridgeHeader("Cartridge header too small.");
}

void Cartridge::check_header_footprint() const
{
        if (data_[0] != 'N' ||
            data_[1] != 'E' ||
            data_[2] != 'S' ||
            data_[3] != 0x1Au) {
                throw InvalidCartridgeHeader("Invalid cartridge header footprint.");
        }
}

std::unique_ptr<MemoryMapper> MemoryMapper::make(Cartridge const& cartridge)
{
        if (cartridge.has_trainer())
                throw InvalidCartridge("Trainers are not supported.");

        switch (cartridge.mmc_id()) {
                case NROM::id: return std::make_unique<NROM>(cartridge);
                default:       throw MemoryMapperNotSupported(cartridge.mmc_id());
        }
}

NROM::NROM(Cartridge const& cartridge)
        : cartridge_(cartridge)
{
        assert(cartridge_.mmc_id() == id);

        if (cartridge_.num_prg_rom_banks() != 1 &&
            cartridge_.num_prg_rom_banks() != 2) {
                throw InvalidCartridgeHeader(
                        "NROM must have either 1 or 2 "
                        "16 KB PRG ROM banks. This one has "s +
                        std::to_string(cartridge_.num_prg_rom_banks()) +
                        "."s);
        }

        if (cartridge_.num_chr_rom_banks() != 1) {
                throw InvalidCartridgeHeader(
                        "NROM must have a single 8 KB "
                        "CHR ROM bank. This one has "s +
                        std::to_string(cartridge_.num_chr_rom_banks()) +
                        "."s);
        }

        if (cartridge_.has_sram()) {
                throw InvalidCartridgeHeader("NROM doesn't have battery-backed SRAM, "
                                             "but this cartridge does.");
        }
}

bool NROM::is_prg_ram(Address address) noexcept
{
        return prg_ram_start <= address && address <= prg_ram_end;
}

bool NROM::address_is_writable_impl(Address address) const noexcept
{
        return is_prg_ram(address);
}

bool NROM::address_is_readable_impl(Address address) const noexcept
{
        return is_prg_ram(address) || Cartridge::is_prg_rom(address);
}

// TODO: I don't know how I should handle CHR-ROM. Something with the PPU?

void NROM::write_byte_impl(Address address, Byte byte)
{
        prg_ram_[address] = byte;
}

Byte NROM::read_byte_impl(Address address)
{
        if (is_prg_ram(address))
                return prg_ram_[address];
        return cartridge_.read_prg_rom_byte(address);
}

}

