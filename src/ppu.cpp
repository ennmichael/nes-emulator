#include <cassert>
#include "ppu.h"

using namespace std::string_literals;

namespace Emulator {

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
        destination(*this, address) = byte;
}

Byte VRAM::read_byte_impl(Address address)
{
        return destination(*this, address);
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

PPU::PPU(ReadableMemory& dma_memory) noexcept
        : dma_memory_(dma_memory)
{}

void PPU::vblank_started()
{}

void PPU::vblank_finished()
{}

Address PPU::base_name_table_address() const noexcept
{
        Address const mult = control_.to_ulong() & 0xC0u;
        Address const offset =
                mult * (VRAM::name_table_size + VRAM::attribute_table_size);
        return VRAM::name_tables_start + offset;
}

Address PPU::address_increment_offset() const noexcept
{
        return (control_.test(2)) ? 0x0020u : 0x0000u;
}

Address PPU::sprite_pattern_table_address() const noexcept
{
        return (control_.test(3)) ? VRAM::pattern_table_size : 0u;
}

Address PPU::background_pattern_table_address() const noexcept
{
        return (control_.test(4)) ? VRAM::pattern_table_size : 0u;
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
        for (unsigned square_x = 0; square_x < 2; ++square_x) {
                for (unsigned square_y = 0; square_y < 2; ++square_y) {
                        paint_background_square(screen, square_x, square_y);
                }
        }

        // FIXME This doesn't respect show_leftmost_background
}

void PPU::paint_background_square(Screen& screen, unsigned square_x, unsigned square_y) noexcept
{
        ByteBitset const attribute = vram_.read_byte(base_name_table_address() +
                                                     VRAM::name_table_size +
                                                     square_x + square_y * 8);
        for (unsigned x = 0; x < 2; ++x) {
                for (unsigned y = 0; y < 2; ++y) {
                        unsigned const first_color_bit = x / 2 + y / 2;
                        std::bitset<2> color_bits = 0;
                        color_bits.set(0, attribute.test(first_color_bit));
                        color_bits.set(1, attribute.test(first_color_bit + 1));
                        unsigned const tile_x = square_x * 2 + x;
                        unsigned const tile_y = square_y * 2 + y;
                        paint_background_tile(screen, tile_x, tile_y, color_bits);
                }
        }
}

void PPU::paint_background_tile(Screen& screen, unsigned tile_x, unsigned tile_y, std::bitset<2> low_color_bits) noexcept
{
        unsigned const tile_index_address = base_name_table_address() + tile_x + tile_y * 8;
        Byte const tile_index = vram_.read_byte(tile_index_address);
        for (unsigned row_num = 0; row_num < background_tile_size; ++row_num) {
                paint_background_tile_row(screen, tile_index, tile_x, tile_y, row_num, low_color_bits);
        }
}

void PPU::paint_background_tile_row(Screen& screen,
                                    Byte tile_index,
                                    unsigned tile_x,
                                    unsigned tile_y,
                                    unsigned row_num,
                                    std::bitset<2> low_color_bits) noexcept
{
        unsigned const tile_row_address = background_pattern_table_address() +
                                          row_num +
                                          tile_index * background_tile_size * 2;
        ByteBitset const first_tile_row = vram_.read_byte(tile_row_address);
        ByteBitset const second_tile_row = vram_.read_byte(tile_row_address + background_tile_size);
        for (unsigned pixel_x = 0; pixel_x < CHAR_BIT; ++pixel_x) {
                ByteBitset palette_index = 0;
                palette_index.set(3, low_color_bits.test(0));
                palette_index.set(2, low_color_bits.test(1));
                palette_index.set(1, second_tile_row.test(pixel_x));
                palette_index.set(0, first_tile_row.test(pixel_x));
                Byte const color = background_color(palette_index.to_ulong());
                screen[tile_x * 8 + pixel_x][tile_y * 8 + row_num] = color;
        }
}

Byte PPU::background_color(unsigned palette_index) noexcept
{
        return vram_.read_byte(VRAM::background_palette_start + palette_index);
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

