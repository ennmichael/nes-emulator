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

class Cartridge : public Memory {
public:
        // TODO There should be no "Header", these can be simple member functions
        struct Header {
                static unsigned constexpr size = 0x10;

                static Header parse(std::string const& path);
                static Header parse(Bytes const& data);

                static Mirroring parse_mirroring(ByteBitset first_control_byte) noexcept;
                static Byte parse_mmc_id(ByteBitset first_control_byte,
                                         ByteBitset second_control_byte) noexcept;

                Byte num_prg_rom_banks = 0;
                Byte num_chr_rom_banks = 0;
                Byte mmc_id = 0;
                bool has_battery_backed_sram = false;
                bool has_trainer = false;
                Mirroring mirroring = Mirroring::horizontal;

                bool has_chr_ram() const noexcept;
        };

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

        static unsigned constexpr nes_file_prg_rom_start = Header::size;

        bool address_is_writable(unsigned address) const noexcept override;
        bool address_is_readable(unsigned address) const noexcept override;

        static UniqueCartridge make(std::string const& path);
        static UniqueCartridge make(Bytes data);
};

class NROM : public Cartridge {
public:
        static Byte constexpr id = 0;

        NROM(Header header, Bytes data);

private:
        void do_write_byte(unsigned address, Byte byte) override;
        Byte do_read_byte(unsigned address) const override;

        Header header_;
        Bytes data_;
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

