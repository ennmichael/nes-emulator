#include "cartridge.h"
#include <utility>

namespace Emulator {

RAMMapperNotSupported::RAMMapperNotSupported(Byte id) noexcept
        : runtime_error("RAM mapper " + std::to_string(id) + " not supported.")
{}

RAMMapper::RAMMapper(std::string const& path)
        : RAMMapper(Utils::read_bytes(path))
{}

RAMMapper::RAMMapper(Bytes data) noexcept
        : data_(std::move(data))
{}

void RAMMapper::write_byte(std::size_t address, Byte byte) noexcept
{
        data_[address] = byte;
}

Byte RAMMapper::read_byte(std::size_t address) const noexcept
{
        return data_[address];
}

bool Cartridge::Header::has_chr_ram() const noexcept
{
        return num_chr_rom_banks == 0;
}

Cartridge::Cartridge(std::string const& path)
        : Cartridge(Utils::read_bytes(path))
{}

Cartridge::Cartridge(Bytes data)
{
        if (data[0] != 'N' ||
            data[1] != 'E' ||
            data[2] != 'S' ||
            data[3] != 0x1A)
                throw InvalidCartridge("Invalid cartridge header footprint.");

        header_ = parse_header(data);
        ram_mapper_ = make_ram_mapper(header_.ram_mapper_id,
                                            std::move(data));
}

auto Cartridge::header() const noexcept -> Header
{
        return header_;
}

auto Cartridge::parse_header(Bytes const& data) -> Header
{
        if (data.size() < 16)
                throw InvalidCartridge("Cartridge header too small.");

        Header header;

        header.num_prg_rom_banks = data[4];
        header.num_chr_rom_banks = data[5];

        ByteBitset const first_control_byte(data[6]);
        ByteBitset const second_control_byte(data[7]);

        header.ram_mapper_id = ram_mapper_id(first_control_byte,
                                                   second_control_byte);
        header.has_battery_backed_sram = first_control_byte.test(1);
        header.has_trainer = first_control_byte.test(2);
        header.mirroring = mirroring(first_control_byte);

        return header;
}

Mirroring Cartridge::mirroring(ByteBitset first_control_byte) noexcept
{
        if (first_control_byte.test(3))
                return Mirroring::four_screen;
        else if (first_control_byte.test(0))
                return Mirroring::vertical;
        return Mirroring::horizontal;
}

Byte Cartridge::ram_mapper_id(ByteBitset first_control_byte,
                                 ByteBitset second_control_byte) noexcept
{
        first_control_byte >>= 4;
        second_control_byte <<= 4;
        ByteBitset const result = first_control_byte | second_control_byte;
        return static_cast<Byte>(result.to_ulong());
}

UniqueRAMMapper Cartridge::make_ram_mapper(Byte ram_mapper_id,
                                                 Bytes data)
{
        switch (ram_mapper_id) {
                case NROM::id: return std::make_unique<NROM>(std::move(data));
                case MMC1::id: return std::make_unique<MMC1>(std::move(data));
                case MMC3::id: return std::make_unique<MMC3>(std::move(data));
        }
        
        throw RAMMapperNotSupported(ram_mapper_id);
}

}

