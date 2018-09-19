// vim: set shiftwidth=8 tabstop=8:

#include <cassert>
#include <algorithm>
#include "ppu.h"

using namespace std::string_literals;

namespace Emulator {

VRAM::VRAM(Mirroring mirroring) noexcept
    : mirroring_(mirroring)
{}

bool VRAM::address_is_writable_impl(Address) const noexcept
{
        return true;
}

bool VRAM::address_is_readable_impl(Address) const noexcept
{
        return true;
}

void VRAM::write_byte_impl(Address address, Byte byte)
{
        if (palettes_start <= address && address <= palettes_end)
                byte &= 0x3F;
        memory_destination(*this, address) = byte;
}

Byte VRAM::read_byte_impl(Address address)
{
        return memory_destination(*this, address);
}

Address VRAM::apply_name_table_mirroring(Address address, Mirroring mirroring) noexcept
{
        if (address > fourth_name_table_end)
                address -= name_tables_real_size;
        switch (mirroring) {
                case Mirroring::horizontal:
                        if ((second_name_table_start <= address && address <= second_name_table_end) ||
                            (fourth_name_table_start <= address && address <= fourth_name_table_end)) {
                                address -= name_table_size + attribute_table_size;
                        }
                        break;
                case Mirroring::vertical:
                        if ((third_name_table_start <= address && address <= third_name_table_end) ||
                            (fourth_name_table_start <= address && address <= fourth_name_table_end)) {
                                address -= 2 * (name_table_size + attribute_table_size);
                        }
                        break;
                case Mirroring::four_screen:
                        break;
                default:
                        assert(false);
        }
        return address % name_tables_real_size;
}

Address VRAM::apply_palettes_mirroring(Address address) noexcept
{
        if (address % 4 == 0)
                return 0;
        return address % palettes_real_size;
}

auto Sprite::priority() const noexcept -> Priority
{
        return (attributes.test(5)) ?
                Priority::above_background : Priority::beneath_background;
}

bool Sprite::flip_vertically() const noexcept
{
        return attributes.test(7);
}

bool Sprite::flip_horizontally() const noexcept
{
        return attributes.test(6);
}

Byte Sprite::palette_index() const noexcept
{
        return attributes.test(0) | (attributes.test(1) << 1);
}

void DoubleRegister::write_byte(Byte byte) noexcept
{
        if (complete_)
                write_high_byte(byte);
        else
                write_low_byte(byte);
        complete_ = !complete_;
}

void DoubleRegister::write_address(Address value) noexcept
{
        value_ = value;
}

void DoubleRegister::increment(Address offset) noexcept
{
        value_ += offset;
}

Address DoubleRegister::read_whole() const noexcept
{
        return value_;
}

Byte DoubleRegister::read_low_byte() const noexcept
{
        return low_byte(value_);
}

Byte DoubleRegister::read_high_byte() const noexcept
{
        return high_byte(value_);
}

bool DoubleRegister::complete() const noexcept
{
        return complete_;
}

void DoubleRegister::write_low_byte(Byte byte) noexcept
{
        value_ |= byte;
}

void DoubleRegister::write_high_byte(Byte byte) noexcept
{
        value_ = static_cast<Address>(byte) << CHAR_BIT;
}

PPU::PPU(Mirroring mirroring, ReadableMemory& dma_memory) noexcept
        : vram_(mirroring)
        , dma_memory_(dma_memory)
{}

void PPU::vblank_started()
{
        status_.set(vblank_flag);
}

void PPU::vblank_finished()
{
        status_.set(vblank_flag);
}

Byte PPU::read_vram_byte(Address address)
{
        return vram_.read_byte(address);
}

Byte PPU::read_oam_byte(Address address)
{
        return oam_[address];
}

Address PPU::read_vram_address_register() const noexcept
{
        return vram_address_.read_whole();
}

Address PPU::base_name_table_address() const noexcept
{
        Address const mult = control_.to_ulong() & 0x03;
        Address const offset =
                mult * (VRAM::name_table_size + VRAM::attribute_table_size);
        return VRAM::name_tables_start + offset;
}

Address PPU::vram_address_increment_offset() const noexcept
{
        return (control_.test(2)) ? 0x0020 : 0x0001;
}

Address PPU::sprite_pattern_table_address() const noexcept
{
        return (control_.test(3)) ? VRAM::pattern_table_size : 0;
}

Address PPU::background_pattern_table_address() const noexcept
{
        return (control_.test(4)) ? VRAM::pattern_table_size : 0;
}

unsigned PPU::sprite_height() const noexcept
{
        return (control_.test(5)) ? 16u : 8u;
}

bool PPU::nmi_enabled() const noexcept
{
        return control_.test(7);
}

bool PPU::greyscale() const noexcept
{
        return mask_.test(0);
}

bool PPU::show_leftmost_background() const noexcept
{
        return mask_.test(1);
}

bool PPU::show_leftmost_sprites() const noexcept
{
        return mask_.test(2);
}

bool PPU::show_background() const noexcept
{
        return mask_.test(3);
}

bool PPU::show_sprites() const noexcept
{
        return mask_.test(4);
}

bool PPU::in_vblank() const noexcept
{
        return status_.test(vblank_flag);
}

Screen PPU::current_screen()
{
        Screen screen {0};
        if (show_background())
                paint_background(screen);
        if (show_sprites())
                paint_sprites(screen);
        return screen;
}

bool PPU::address_is_writable_impl(Address address) const noexcept
{
        return address == control_register ||
               address == mask_register ||
               address == oam_address_register ||
               address == oam_data_register ||
               address == scroll_register ||
               address == vram_address_register ||
               address == vram_data_register ||
               address == oam_dma_register;
}

bool PPU::address_is_readable_impl(Address address) const noexcept
{
        return address == status_register ||
               address == oam_data_register ||
               address == vram_data_register;
}

void PPU::write_byte_impl(Address address, Byte byte)
{
        switch (address) {
                case control_register:
                        control_ = byte;
                        break;

                case mask_register:
                        mask_ = byte;
                        break;

                case oam_address_register:
                        oam_address_ = byte;
                        break;

                case oam_data_register:
                        oam_[oam_address_] = byte;
                        increment_oam_address();
                        break;

                case scroll_register:
                        scroll_.write_byte(byte);
                        break;

                case vram_address_register:
                        vram_address_.write_byte(byte);
                        break;

                case vram_data_register:
                        vram_.write_byte(vram_address_.read_whole(), byte);
                        increment_vram_address();
                        break;

                default:
                        assert(false);
                        break;
        }
}

Byte PPU::read_byte_impl(Address address)
{
        switch (address) {
                case status_register:
                        return status_.to_ulong();

                case oam_data_register:
                        return oam_[oam_address_];

                case vram_data_register:
                        {
                                // This behaviour isn't 100% correct, because reading
                                // further than 0x3EFF shouldn't read palette memory;
                                // it should wrap back. For now, I don't really care.
                                Byte const result = vram_data_buffer_;
                                vram_data_buffer_ = vram_.read_byte(vram_address_.read_whole());
                                increment_vram_address();
                                return result;
                        }

                default:
                        assert(false);
                        return 0;
        }
}

void PPU::paint_background(Screen& screen)
{
        auto const paint_4x4_tile_square = [&](Byte palette_index, Byte tile_index)
        {
                auto const paint_tile = [&](Address tile_address)
                {
                        Tile const tile = read_tile(tile_address);
                        Byte const x = tile_address % 32;
                        Byte const y = (tile_address - x) % 32;
                        paint_background_tile(screen, VRAM::background_palette_start, palette_index, x, y, tile);
                };

                Address const upper_left_tile_address = background_tile_address(tile_index);
                paint_tile(upper_left_tile_address);
                paint_tile(upper_left_tile_address + 1);
                paint_tile(upper_left_tile_address + 32);
                paint_tile(upper_left_tile_address + 32 + 1);
        };

        Address const name_table_address = base_name_table_address();
        Address const attribute_table_address = name_table_address + VRAM::name_table_size;
        for (Address attribute_address = attribute_table_address;
             attribute_address < attribute_table_address + VRAM::attribute_table_size;
             ++attribute_address) {
                Address const attribute_byte = vram_.read_byte(attribute_address);
                Address const name_table_address = attribute_address - VRAM::name_table_size +
                                                  4 * (attribute_address - attribute_table_address);
                paint_4x4_tile_square(attribute_byte & 0x03, vram_.read_byte(name_table_address));
                paint_4x4_tile_square((attribute_byte >> 2) & 0x03, vram_.read_byte(name_table_address + 2));
                paint_4x4_tile_square((attribute_byte >> 4) & 0x03, vram_.read_byte(name_table_address + 64));
                paint_4x4_tile_square((attribute_byte >> 6) & 0x03, vram_.read_byte(name_table_address + 64 + 2));
        }
}

Address PPU::background_tile_address(Byte tile_index) noexcept
{
        return background_pattern_table_address() + tile_index;
}

Tile PPU::read_tile(Address tile_address)
{
        Tile tile {0};
        for (unsigned y = 0; y < 8; ++y) {
                Byte const first_plane = vram_.read_byte(tile_address + y);
                Byte const second_plane = vram_.read_byte(tile_address + y + 1);
                for (unsigned x = 0; x < 8; ++x) {
                        bool const first_bit = get_bit(first_plane, x);
                        bool const second_bit = get_bit(second_plane, x);
                        tile[y][x] = first_bit + second_bit;
                }
        }
        return tile;
}

Sprites PPU::read_sprites()
{
        Sprites sprites {0};
        for (unsigned i = 0; i < oam_size / sprite_size; ++i) {
                Address const address = i * sprite_size;
                sprites[i] = Sprite {
                        .y = vram_.read_byte(address),
                        .tile_index = vram_.read_byte(address + 1),
                        .attributes = vram_.read_byte(address + 2),
                        .x = vram_.read_byte(address + 3)
                };
        }
        return sprites;
}

void PPU::paint_sprites(Screen& screen)
{}

// Possibly difficult: how to reuse code when painting sprites

void PPU::paint_background_tile(Screen& screen, Address palette_address,
                                Byte high_palette_index_bits, Byte x, Byte y, Tile const& tile)
{
        for (unsigned y_offset = 0; y_offset < tile_height; ++y_offset) {
                for (unsigned x_offset = 0; x_offset < tile_width; ++x_offset) {
                        Byte const palette_index = tile[y_offset][x_offset] | (high_palette_index_bits << 2);
                        Byte const color = vram_.read_byte(palette_address + palette_index);
                        screen[x][y] = color;
                }
        }
}

/* TODO
 * Possibly correct code, but currently unused
Byte PPU::sprite_color(unsigned palette_index) noexcept
{
        return vram_.read_byte(VRAM::background_palette_start + palette_index);
}
*/

void PPU::increment_oam_address() noexcept
{
        ++oam_address_;
}

void PPU::increment_vram_address() noexcept
{
        vram_address_.increment(vram_address_increment_offset());
}

void PPU::execute_dma(Byte source)
{
        for (Address i = oam_address_; i < static_cast<Address>(oam_address_) + oam_size; ++i) {
                Address const j = i % byte_max;
                Address const source_address = source * oam_size + j;
                oam_[j] = dma_memory_.read_byte(source_address);
        }
}

}

