#pragma once

#include "utils.h"
#include "mem.h"
#include <stdexcept>
#include <vector>
#include <memory>
#include <string>

namespace Emulator {

class MemoryMapperNotSupported : public std::runtime_error {
public:
        explicit MemoryMapperNotSupported(Byte id) noexcept;
};

class InvalidCartridgeHeader : public std::runtime_error {
public:
        using runtime_error::runtime_error;
};

class InvalidCartridge : public std::runtime_error {
public:
        using runtime_error::runtime_error;
};

enum class Mirroring {
        horizontal,
        vertical,
        four_screen
};

class Cartridge;

using UniqueCartridge = std::unique_ptr<Cartridge>;

struct NESFile {
        static unsigned constexpr header_size = 0x10;
        static unsigned constexpr prg_rom_start = header_size;

        explicit NESFile(std::string const& path);
        explicit NESFile(Bytes new_data);

        Byte num_prg_rom_banks() const noexcept;
        Byte num_chr_rom_banks() const noexcept;
        Byte mmc_id() const noexcept;
        bool has_sram() const noexcept;
        bool has_trainer() const noexcept;
        Mirroring mirroring() const noexcept;
        bool has_chr_ram() const noexcept;

        ByteBitset first_control_byte() const noexcept;
        ByteBitset second_control_byte() const noexcept;
        
        Bytes data;

private:
        void check_data_size() const;
        void check_header_footprint() const;
};

class Cartridge : public Memory {
public:
        static unsigned constexpr prg_ram_start = 0x6000;
        static unsigned constexpr prg_ram_bank_size = 0x2000;
        static unsigned constexpr prg_ram_end = prg_ram_start + prg_ram_bank_size;

        static unsigned constexpr prg_rom_bank_size = 0x4000;
        static unsigned constexpr prg_rom_lower_bank_start = 0x8000;
        static unsigned constexpr prg_rom_lower_bank_end = 
                prg_rom_lower_bank_start + prg_rom_bank_size;
        static unsigned constexpr prg_rom_upper_bank_start = prg_rom_lower_bank_end;
        static unsigned constexpr prg_rom_upper_bank_end =
                prg_rom_upper_bank_start + prg_rom_bank_size;

        static UniqueCartridge make(std::string const& path);
        static UniqueCartridge make(NESFile nes_file);
};

class NROM : public Cartridge {
public:
        static Byte constexpr id = 0;

        explicit NROM(NESFile nes_file);

        static bool address_is_writable(unsigned address) const noexcept;
        static bool address_is_readable(unsigned address) const noexcept;

        void write_byte(unsigned address, Byte byte) override;
        Byte read_byte(unsigned address) const override;

private:
        NESFile nes_file_;
        std::array<Byte, 0x2000> prg_ram_ {0};
};

class MMC1 : public Cartridge {
public:
        static Byte constexpr id = 1;
};

class MMC3 : public Cartridge {
public:
        static Byte constexpr id = 4;
};

}

