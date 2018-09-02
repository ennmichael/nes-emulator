#pragma once

#include "utils.h"

namespace Emulator {

class VRAM : public Memory {
public:
        static unsigned constexpr size = 0x10000;

        static unsigned constexpr pattern_tables_start = 0x0000;
        static unsigned constexpr pattern_tables_end = 0x2000;
        static unsigned constexpr pattern_table_size = 0x1000;
        static unsigned constexpr pattern_tables_size =
                pattern_tables_end - pattern_tables_start;

        static unsigned constexpr name_table_size = 0x03C0;
        static unsigned constexpr attribute_table_size = 0x0040;
        static unsigned constexpr name_tables_start = pattern_tables_end;
        static unsigned constexpr name_tables_end = 0x3F00;
        static unsigned constexpr name_tables_size =
                name_tables_end - name_tables_start;
        static unsigned constexpr name_tables_real_size =
                0x3000u - name_tables_start;

        static unsigned constexpr palette_size = 0x0010;
        static unsigned constexpr background_palette_start = name_tables_end;
        static unsigned constexpr background_palette_end =
                background_palette_start + palette_size;
        static unsigned constexpr sprite_palette_start = background_palette_end;
        static unsigned constexpr sprite_palette_end =
                sprite_palette_start + palette_size;
        static unsigned constexpr palettes_real_size =
                sprite_palette_end - background_palette_start;
        static unsigned constexpr palettes_start = background_palette_start;
        static unsigned constexpr palettes_end = 0x4000;
        static unsigned constexpr palettes_size = palettes_end - palettes_start;

        static bool address_is_accessible(unsigned address) noexcept;
        bool address_is_writable(unsigned address) const noexcept override;
        bool address_is_readable(unsigned address) const noexcept override;

        void write_byte(unsigned address, Byte byte) override;
        Byte read_byte(unsigned address) override;
        Byte read_byte(unsigned address) const;

private:
        template <class Self>
        static auto& destination(Self& self, unsigned address) noexcept
        {
                if (pattern_tables_start <= address && address < pattern_tables_end)
                        return self.pattern_tables_[address];
                else if (name_tables_start <= address && address < name_tables_end)
                        return self.name_tables_[address % name_tables_real_size];
                else if (palettes_start <= address && address < palettes_end)
                        return self.palettes_[address % palettes_real_size];
                else
                        return destination(self, address % palettes_end);
        }

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
        std::bitset<2> color_bits() const noexcept;
};

static unsigned constexpr oam_size = 0x0100;
using OAM = std::array<Byte, oam_size>;

unsigned constexpr screen_width = 256;
unsigned constexpr screen_height = 240;
using Screen = Matrix<Byte, screen_width, screen_height>;

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
        unsigned value_ = 0;
        bool complete_ = true;
};

class PPU : public Memory {
public:
        static unsigned constexpr control_register = 0x2000;
        static unsigned constexpr mask_register = 0x2001;
        static unsigned constexpr status_register = 0x2002;
        static unsigned constexpr oam_address_register = 0x2003;
        static unsigned constexpr oam_data_register = 0x2004;
        static unsigned constexpr scroll_register = 0x2005;
        static unsigned constexpr vram_address_register = 0x2006;
        static unsigned constexpr vram_data_register = 0x2007;
        static unsigned constexpr oam_dma_register = 0x4014;

        static unsigned constexpr sprite_width = 8;
        static unsigned constexpr background_square_size = 16;
        static unsigned constexpr background_tile_size = 8;
        static unsigned constexpr background_tiles_per_square =
                background_square_size / background_tile_size;

        explicit PPU(ReadableMemory& dma_memory) noexcept;

        void vblank_started();
        void vblank_finished();

        bool address_is_writable(unsigned address) const noexcept override;
        bool address_is_readable(unsigned address) const noexcept override;

        void write_byte(unsigned address, Byte byte);
        Byte read_byte(unsigned address);

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

        Screen screen() const;
        
private:
        [[noreturn]] static void throw_not_writable(std::string const& register_name,
                                                    unsigned address);
        [[noreturn]] static void throw_not_readable(std::string const& register_name,
                                                    unsigned address);
        [[noreturn]] static void throw_not_valid(unsigned address);

        void paint_background(Screen& screen) const noexcept;
        void paint_background_square(Screen& screen,
                                     unsigned square_x,
                                     unsigned square_y) const noexcept;
        void paint_background_tile(Screen& screen,
                                   unsigned tile_x,
                                   unsigned tile_y,
                                   std::bitset<2> low_color_bits) const noexcept;
        void paint_background_tile_row(Screen& screen,
                                       Byte tile_index,
                                       unsigned tile_x,
                                       unsigned tile_y,
                                       unsigned row_num,
                                       std::bitset<2> low_color_bits) const noexcept;

        Byte background_color(unsigned palette_index) const noexcept;
        Byte sprite_color(unsigned palette_index) const noexcept;

        void increment_oam_address() noexcept;
        void increment_vram_address() noexcept;
        void execute_dma(Byte source);

        ByteBitset control_ = 0;
        ByteBitset mask_ = 0;
        ByteBitset status_ = 0;
        Byte oam_address_ = 0;
        DoubleWriteRegister scroll_;
        DoubleWriteRegister vram_address_;
        Byte vram_data_buffer_ = 0;
        VRAM vram_;
        OAM oam_ {0};
        ReadableMemory& dma_memory_;
};

using UniquePPU = std::unique_ptr<PPU>;

}

