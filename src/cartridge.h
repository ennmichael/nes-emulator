#pragma once

#include "utils.h"
#include <stdexcept>
#include <vector>
#include <memory>
#include <string>

namespace Emulator {

class RAMMapperNotSupported : public std::runtime_error {
public:
        explicit RAMMapperNotSupported(Byte id) noexcept;
};

class InvalidCartridge : public std::runtime_error {
public:
        using std::runtime_error::runtime_error;
};

// FIXME This is not what this interface should look like
class RAMMapper {
public:
        RAMMapper(std::string const& path);
        RAMMapper(Bytes data) noexcept;

        RAMMapper(RAMMapper const& other) = delete;
        RAMMapper(RAMMapper&& other) = delete;

        RAMMapper& operator=(RAMMapper const& other) = delete;
        RAMMapper& operator=(RAMMapper&& other) = delete;

        virtual ~RAMMapper() = default;

        virtual void write_byte(std::size_t address, Byte byte) noexcept;
        virtual Byte read_byte(std::size_t address) const noexcept;

private:
        Bytes data_;
};

using UniqueRAMMapper = std::unique_ptr<RAMMapper>;

class NROM : public RAMMapper {
public:
        static Byte constexpr id = 0;

        using RAMMapper::RAMMapper;
};

class MMC1 : public RAMMapper {
public:
       static Byte constexpr id = 1;

       using RAMMapper::RAMMapper;
};

class MMC3 : public RAMMapper {
public:
        static Byte constexpr id = 4;

        using RAMMapper::RAMMapper;
};

enum class Mirroring {
        horizontal,
        vertical,
        four_screen
};

class Cartridge {
public:
        struct Header {
                Byte num_prg_rom_banks = 0;
                Byte num_chr_rom_banks = 0;
                Byte ram_mapper_id = NROM::id;
                bool has_battery_backed_sram = false;
                bool has_trainer = false;
                Mirroring mirroring = Mirroring::horizontal;

                bool has_chr_ram() const noexcept;
        };

        explicit Cartridge(std::string const& path);
        explicit Cartridge(Bytes data);

        Header header() const noexcept;

        void write(std::size_t address, Byte byte) noexcept;
        Byte read(std::size_t address) const noexcept;

private:
        static Header parse_header(Bytes const& data);
        static Mirroring mirroring(ByteBitset first_control_byte) noexcept;
        static Byte ram_mapper_id(ByteBitset first_control_byte,
                                     ByteBitset second_control_byte) noexcept;
        static UniqueRAMMapper make_ram_mapper(Byte ram_mapper_id,
                                                     Bytes data);

        Header header_;
        UniqueRAMMapper ram_mapper_ = nullptr;
};

}

