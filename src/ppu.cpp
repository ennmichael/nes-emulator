// vim: set shiftwidth=8 tabstop=8:

#include <cassert>
#include "ppu.h"

using namespace std::string_literals;

namespace Emulator {

VRAM::VRAM(Mirroring mirroring) noexcept
    : mirroring_(mirroring)
{}

Tile VRAM::read_tile(Address address)
{
        Tile tile {0};
        for (unsigned y = 0; y < 8; ++y) {
                Byte const first_plane = read_byte(address + y);
                Byte const second_plane = read_byte(address + y + 1);
                for (unsigned x = 0; x < 8; ++x) {
                        Byte const first_bit = Utils::bit(first_plane, x);
                        Byte const second_bit = Utils::bit(second_plane, x);
                        tile[x][y] = first_bit + second_bit;
                }
        }
        return tile;
}

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
        memory_destination(*this, address) = byte;
}

Byte VRAM::read_byte_impl(Address address)
{
        return memory_destination(*this, address);
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

std::bitset<2> Sprite::color_bits() const noexcept
{
        std::bitset<2> result;
        result.set(0, attributes.test(0));
        result.set(1, attributes.test(1));
        return result;
}

void DoubleWriteRegister::write_byte(Byte byte) noexcept
{
        if (complete_)
                value_ = byte;
        else
                value_ |= static_cast<Address>(byte) << CHAR_BIT;
        complete_ = !complete_;
}

void DoubleWriteRegister::write_address(Address value) noexcept
{
        value_ = value;
}

void DoubleWriteRegister::increment(Address offset) noexcept
{
        value_ += offset;
}

Address DoubleWriteRegister::read_address() const noexcept
{
        return value_;
}

Byte DoubleWriteRegister::read_low_byte() const noexcept
{
        return Utils::low_byte(value_);
}

Byte DoubleWriteRegister::read_high_byte() const noexcept
{
        return Utils::high_byte(value_);
}

bool DoubleWriteRegister::complete() const noexcept
{
        return complete_;
}

PPU::PPU(Mirroring mirroring, ReadableMemory& dma_memory) noexcept
        : vram_(mirroring)
        , dma_memory_(dma_memory)
{}

void PPU::vblank_started()
{}

void PPU::vblank_finished()
{}

Address PPU::base_name_table_address() const noexcept
{
        Address const mult = control_.to_ulong() & 0x03;
        Address const offset =
                mult * (VRAM::name_table_size + VRAM::attribute_table_size);
        return VRAM::name_tables_start + offset;
}

Address PPU::address_increment_offset() const noexcept
{
        return (control_.test(2)) ? 0x0020 : 0x0000;
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

Screen PPU::screen()
{
        Screen screen {0};
        if (show_background())
                paint_background(screen);
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
                        break;

                case scroll_register:
                        scroll_.write_byte(byte);
                        break;

                case vram_address_register:
                        vram_address_.write_byte(byte);
                        break;

                case vram_data_register:
                        vram_.write_byte(vram_address_.read_address(), byte);
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
                        {
                                Byte const result = oam_[oam_address_];
                                increment_oam_address();
                                return result;
                        }

                case vram_data_register:
                        {
                                Byte const result = vram_data_buffer_;
                                vram_data_buffer_ =
                                        vram_.read_byte(vram_address_.read_address());
                                increment_vram_address();
                                return result;
                        }

                default:
                        assert(false);
                        return 0;
        }
}

void PPU::paint_background(Screen& screen) noexcept
{
        // FIXME This doesn't respect show_leftmost_background
        auto const draw_tile = [&](Byte palette, Address tile_address)
        {
                auto const tile = vram_.read_tile(tile_address);
                unsigned const x = tile_address % 32;
                unsigned const y = (tile_address - x) / 32;
                for (unsigned y_offset = 0; y_offset < tile_height; ++y_offset) {
                        for (unsigned x_offset = 0; x_offset < tile_width; ++x_offset) {
                                Byte const palette_index = tile[y_offset][x_offset] | (palette << 2);
                                Byte const color = vram_.read_byte(VRAM::background_palette_start + palette_index);
                                screen[x][y] = color;
                        }
                }
        };

        auto const draw_4x4_tile_square = [&](Byte palette, Address upper_left_tile_address)
        {
                draw_tile(palette, upper_left_tile_address);
                draw_tile(palette, upper_left_tile_address + 1);
                draw_tile(palette, upper_left_tile_address + 32);
                draw_tile(palette, upper_left_tile_address + 32 + 1);
        };

        Address const name_table_address = base_name_table_address();
        Address const attribute_table_address = name_table_address + VRAM::name_table_size;
        for (Address attribute_address = attribute_table_address;
             attribute_address < attribute_table_address + VRAM::attribute_table_size;
             ++attribute_address) {
                Address const attribute_byte = vram_.read_byte(attribute_address);
                Address const base_tile_address = attribute_address - VRAM::name_table_size +
                                                  4 * (attribute_address - attribute_table_address);
                draw_4x4_tile_square(attribute_byte & 0x03, base_tile_address);
                draw_4x4_tile_square((attribute_byte >> 2) & 0x03, base_tile_address + 2);
                draw_4x4_tile_square((attribute_byte >> 4) & 0x03, base_tile_address + 64);
                draw_4x4_tile_square((attribute_byte >> 6) & 0x03, base_tile_address + 64 + 2);
        }
}

Byte PPU::sprite_color(unsigned palette_index) noexcept
{
        return vram_.read_byte(VRAM::background_palette_start + palette_index);
}

void PPU::increment_oam_address() noexcept
{
        oam_address_ += address_increment_offset();
}

void PPU::increment_vram_address() noexcept
{
        vram_address_.increment(address_increment_offset());
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

