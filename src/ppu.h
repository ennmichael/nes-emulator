#pragma once

#include "mem.h"
#include <iterator>

namespace Emulator {

class VRAM : public Memory {
public:
        static unsigned constexpr size = 0x10000u;

        static unsigned constexpr pattern_tables_start = 0x0000u;
        static unsigned constexpr pattern_tables_end = 0x2000u;
        static unsigned constexpr pattern_table_size = 0x1000u;
        static unsigned constexpr pattern_tables_size =
                pattern_tables_end - pattern_tables_start;

        static unsigned constexpr name_table_size = 0x03C0u;
        static unsigned constexpr attribute_table_site = 0x0040u;
        static unsigned constexpr name_tables_start = pattern_tables_end;
        static unsigned constexpr name_tables_end = 0x3F00u;
        static unsigned constexpr name_tables_size =
                name_tables_end - name_tables_start;
        static unsigned constexpr name_tables_real_size =
                0x3000u - name_tables_start;

        static unsigned constexpr palette_size = 0x0010u;
        static unsigned constexpr image_palette_start = name_tables_end;
        static unsigned constexpr image_palette_end =
                image_palette_start + palette_size;
        static unsigned constexpr sprite_palette_start = image_palette_end;
        static unsigned constexpr sprite_palette_end =
                sprite_palette_start + palette_size;
        static unsigned constexpr palettes_real_size =
                sprite_palette_end - image_palette_start;
        static unsigned constexpr palettes_start = image_palette_start;
        static unsigned constexpr palettes_end = 0x4000u;
        static unsigned constexpr palettes_size = palettes_end - palettes_start;

        static bool address_is_valid(unsigned address) noexcept;

        void write_byte(unsigned address, Byte byte) override;
        Byte read_byte(unsigned address) const override;

private:
        template <class Self>
        static auto& destination(Self& self, unsigned address) noexcept;

        std::array<Byte, pattern_tables_size> pattern_tables_ {0};
        std::array<Byte, name_tables_real_size> name_tables_ {0};
        std::array<Byte, palettes_real_size> palettes_ {0};
};

struct Sprite {
        Byte x = 0;
        Byte y = 0;
        Byte tile_index = 0;
        ByteBitset attributes = 0;

        enum class Priority {
                beneath_background,
                above_background
        };

        Priority priority() const noexcept;
        bool flip_vertically() const noexcept;
        bool flip_horizontally() const noexcept;
        ByteBitset color_bits() const noexcept;
};

class DoubleWriteRegister {
public:
        void write_half(Byte byte) noexcept;
        void write_whole(unsigned value) noexcept;
        void increment(unsigned offset) noexcept;

        unsigned read_whole() const noexcept;
        Byte read_low_byte() const noexcept;
        Byte read_high_byte() const noexcept;

        bool complete() const noexcept;

private:
        Byte low_byte_ = 0;
        Byte high_byte_ = 0;
        bool complete_ = true;
};

std::size_t constexpr screen_width = 256;
std::size_t constexpr screen_height = 240;
using Screen = std::array<std::array<Byte, screen_width>, screen_height>;

class PPU : public Memory {
public:
        static unsigned constexpr control_register = 0x2000u;
        static unsigned constexpr mask_register = 0x2001u;
        static unsigned constexpr status_register = 0x2002u;
        static unsigned constexpr oam_address_register = 0x2003u;
        static unsigned constexpr oam_data_register = 0x2004u;
        static unsigned constexpr scroll_register = 0x2005u;
        static unsigned constexpr vram_address_register = 0x2006u;
        static unsigned constexpr vram_data_register = 0x2007u;
        static unsigned constexpr oam_dma_register = 0x4014u;

        static unsigned constexpr oam_size = 0x0100u;
        static unsigned constexpr sprite_width = 8;

        explicit PPU(Memory const& dma_memory) noexcept;

        PPU(PPU const& other) = delete;
        PPU(PPU&& other) = delete;
        PPU& operator=(PPU const& other) = delete;
        PPU& operator=(PPU&& other) = delete;
        ~PPU() = default;

        void vblank_started();
        void vblank_finished();

        void write_byte(unsigned address, Byte byte) override;
        Byte read_byte(unsigned address) const override;

        unsigned base_name_table_address() const noexcept;
        unsigned address_increment_offset() const noexcept;
        unsigned sprite_pattern_table_address() const noexcept;
        unsigned background_pattern_table_address() const noexcept;
        unsigned sprite_height() const noexcept;
        bool nmi_enabled() const noexcept;
        bool greyscale() const noexcept;
        bool show_leftmost_background() const noexcept;
        bool show_leftmost_sprites() const noexcept;
        bool show_background() const noexcept;
        bool show_sprites() const noexcept;

        // TODO What exactly is control bit 6 for?

        Screen screen() const;
        
private:
        [[noreturn]] static void throw_not_writable(std::string const& register_name,
                                                    unsigned address);
        [[noreturn]] static void throw_not_readable(std::string const& register_name,
                                                    unsigned address);
        [[noreturn]] static void throw_not_valid(unsigned address);

        void increment_oam_address() noexcept;
        void increment_vram_address() noexcept;

        void write_vram(unsigned address, Byte byte);
        Byte read_vram(unsigned address) const;

        void execute_dma(Byte source);

        // TODO First access of something is ignored, check that.
        ByteBitset control_ = 0;
        ByteBitset mask_ = 0;
        ByteBitset status_ = 0;
        Byte oam_address_ = 0;
        Byte oam_data_buffer_ = 0; // Something to do with this?
        DoubleWriteRegister scroll_;
        DoubleWriteRegister vram_address_;
        VRAM vram_;
        std::array<Byte, oam_size> oam_;
        Memory const& dma_memory_;
};

}

