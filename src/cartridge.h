#pragma once

#include "utils.h"
#include <stdexcept>
#include <vector>
#include <memory>
#include <string>

namespace Emulator {

class MemoryMapperNotSupported : public std::runtime_error {
public:
        explicit MemoryMapperNotSupported(Byte id) noexcept;
};

class InvalidCartridge : public std::runtime_error {
public:
        using std::runtime_error::runtime_error;
};

class MemoryMapper {
public:
        MemoryMapper(std::string const& path);
        MemoryMapper(std::vector<Byte> data) noexcept;

        MemoryMapper(MemoryMapper const& other) = delete;
        MemoryMapper(MemoryMapper&& other) = delete;

        MemoryMapper& operator=(MemoryMapper const& other) = delete;
        MemoryMapper& operator=(MemoryMapper&& other) = delete;

        virtual ~MemoryMapper() = default;

        virtual void write_byte(std::size_t address, Byte byte) noexcept;
        virtual Byte read_byte(std::size_t address) const noexcept;

private:
        std::vector<Byte> data_;
};

using UniqueMemoryMapper = std::unique_ptr<MemoryMapper>;

class NROM : public MemoryMapper {
public:
        static Byte constexpr id = 0;

        using MemoryMapper::MemoryMapper;
};

class MMC1 : public MemoryMapper {
public:
       static Byte constexpr id = 1;

       using MemoryMapper::MemoryMapper;
};

class MMC3 : public MemoryMapper {
public:
        static Byte constexpr id = 4;

        using MemoryMapper::MemoryMapper;
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
                Byte memory_mapper_id = NROM::id;
                bool has_battery_backed_sram = false;
                bool has_trainer = false;
                Mirroring mirroring = Mirroring::horizontal;

                bool has_chr_ram() const noexcept;
        };

        explicit Cartridge(std::string const& path);
        explicit Cartridge(std::vector<Byte> data);

        Header header() const noexcept;

        void write(std::size_t address, Byte byte) noexcept;
        Byte read(std::size_t address) const noexcept;

private:
        static Header parse_header(std::vector<Byte> const& data);
        static Mirroring mirroring(ByteBitset first_control_byte) noexcept;
        static Byte memory_mapper_id(ByteBitset first_control_byte,
                                     ByteBitset second_control_byte) noexcept;
        static UniqueMemoryMapper make_memory_mapper(Byte memory_mapper_id,
                                                     std::vector<Byte> data);

        Header header_;
        UniqueMemoryMapper memory_mapper_ = nullptr;
};

}

