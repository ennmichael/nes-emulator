#include "ppu.h"

using namespace std::string_literals;

namespace Emulator {

bool VRAM::address_is_accessible(unsigned address) noexcept
{
        return address < size;
}

bool VRAM::address_is_writable(unsigned address) const noexcept
{
        return address_is_accessible(address);
}

bool VRAM::address_is_readable(unsigned address) const noexcept
{
        return address_is_accessible(address);
}

void VRAM::write_byte(unsigned address, Byte byte)
{
        if (!address_is_accessible(address)) {
                throw InvalidAddress("Can't write to VRAM address"s +
                                     Utils::format_address(address));
        }

        destination(*this, address) = byte;
}

Byte VRAM::read_byte(unsigned address) const
{
        if (!address_is_accessible(address)) {
                throw InvalidAddress("Can't read VRAM address"s +
                                     Utils::format_address(address));
        }

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

void DoubleWriteRegister::write_half(Byte byte) noexcept
{
        if (complete_)
                value_ = byte;
        else
                value_ |= static_cast<unsigned>(byte) << CHAR_BIT;
        complete_ = !complete_;
}

void DoubleWriteRegister::write_whole(unsigned value) noexcept
{
        value_ = value;
}

void DoubleWriteRegister::increment(unsigned offset) noexcept
{
        value_ += offset;
}

unsigned DoubleWriteRegister::read_whole() const noexcept
{
        return value_;
}

Byte DoubleWriteRegister::read_low_byte() const noexcept
{
        return value_ & Utils::low_byte_mask;
}

Byte DoubleWriteRegister::read_high_byte() const noexcept
{
        return value_ & Utils::high_byte_mask;
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

bool PPU::address_is_writable(unsigned address) const noexcept
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

bool PPU::address_is_readable(unsigned address) const noexcept
{
        return address == status_register ||
               address == oam_data_register ||
               address == vram_data_register;
}

void PPU::write_byte(unsigned address, Byte byte)
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
                        scroll_.write_half(byte);
                        break;

                case vram_address_register:
                        vram_address_.write_half(byte);
                        break;

                case vram_data_register:
                        vram_.write_byte(vram_address_.read_whole(), byte);
                        break;

                case oam_dma_register:
                        execute_dma(byte);
                        break;

                case status_register:
                        throw_not_readable("status"s, status_register);

                default:
                        throw_not_valid(address);
        }
}

Byte PPU::read_byte(unsigned address)
{
        switch (address) {
                case control_register:
                        throw_not_readable("control"s, control_register);

                case mask_register:
                        throw_not_readable("mask"s, mask_register);

                case status_register:
                        return Utils::to_byte(status_);

                case oam_address_register:
                        throw_not_readable("OAM address"s, oam_address_register);

                case oam_data_register:
                        {
                                Byte const result = oam_[oam_address_];
                                increment_oam_address();
                                return result;
                        }

                case scroll_register:
                        throw_not_readable("scroll"s, scroll_register);

                case vram_address_register:
                        throw_not_readable("VRAM address"s, vram_address_register);

                case vram_data_register:
                        {
                                Byte const result = vram_data_buffer_;
                                vram_data_buffer_ =
                                        vram_.read_byte(vram_address_.read_whole());
                                increment_vram_address();
                                return result;
                        }

                case oam_dma_register:
                        throw_not_readable("OAM DMA"s, oam_dma_register);

                default:
                        throw_not_valid(address);
        }
}

void PPU::throw_not_writable(std::string const& register_name,
                             unsigned address)
{
        throw InvalidAddress("PPU "s + register_name + " register ("s +
                             Utils::format_address(address) + ") is not writable."s);
}

void PPU::throw_not_readable(std::string const& register_name,
                             unsigned address)
{
        throw InvalidAddress("PPU "s + register_name + " register ("s +
                             Utils::format_address(address) + ") is not readable."s);
}

void PPU::throw_not_valid(unsigned address)
{
        throw InvalidAddress(Utils::format_address(address) +
                             " is not a valid PPU memory address."s);
}

unsigned PPU::base_name_table_address() const noexcept
{
        unsigned const mult = control_.to_ulong() & 0xC0u;
        unsigned const offset =
                mult * (VRAM::name_table_size + VRAM::attribute_table_size);
        return VRAM::name_tables_start + offset;
}

unsigned PPU::address_increment_offset() const noexcept
{
        return (control_.test(2)) ? 0x0020u : 0x0000u;
}

unsigned PPU::sprite_pattern_table_address() const noexcept
{
        return (control_.test(3)) ? VRAM::pattern_table_size : 0u;
}

unsigned PPU::background_pattern_table_address() const noexcept
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

Screen PPU::screen() const
{
        Screen screen {0};
        if (show_background())
                paint_background(screen);
        return screen;
}

void PPU::paint_background(Screen& screen) const noexcept
{
        for (unsigned square_x = 0; square_x < 8; ++square_x) {
                for (unsigned square_y = 0; square_y < 8; ++square_y) {
                        paint_background_square(screen, square_x, square_y);
                }
        }

        // FIXME This doesn't respect show_leftmost_background
}

void PPU::paint_background_square(Screen& screen,
                                  unsigned square_x,
                                  unsigned square_y) const noexcept
{
        ByteBitset const attribute = vram_.read_byte(base_name_table_address() +
                                                     VRAM::name_table_size +
                                                     square_x + square_y * 8u);

        for (unsigned x = 0; x < background_tiles_per_square; ++x) {
                for (unsigned y = 0; y < background_tiles_per_square; ++y) {
                        unsigned const first_color_bit = x / 2u + y / 2u;
                        unsigned const tile_x =
                                square_x * background_tiles_per_square + x;
                        unsigned const tile_y =
                                square_y * background_tiles_per_square + y;
                        std::bitset<2> color_bits = 0;
                        color_bits.set(0, attribute.test(first_color_bit));
                        color_bits.set(1, attribute.test(first_color_bit + 1));
                        paint_background_tile(screen,
                                              tile_x, tile_y,
                                              color_bits);
                }
        }
}

void PPU::paint_background_tile(Screen& screen,
                                unsigned tile_x,
                                unsigned tile_y,
                                std::bitset<2> low_color_bits) const noexcept
{
        unsigned const tile_index_address = base_name_table_address() +
                                            tile_x + tile_y * 8u;
        Byte const tile_index = vram_.read_byte(tile_index_address);
        for (unsigned row_num = 0; row_num < background_tile_size; ++row_num) {
                paint_background_tile_row(screen, tile_index, tile_x, tile_y,
                                          row_num, low_color_bits);
        }
}

void PPU::paint_background_tile_row(Screen& screen,
                                    Byte tile_index,
                                    unsigned tile_x,
                                    unsigned tile_y,
                                    unsigned row_num,
                                    std::bitset<2> low_color_bits) const noexcept
{
        unsigned const tile_row_address = background_pattern_table_address() +
                                          row_num +
                                          tile_index * background_tile_size * 2;
        ByteBitset const first_tile_row = vram_.read_byte(tile_row_address);
        ByteBitset const second_tile_row
                = vram_.read_byte(tile_row_address + background_tile_size);

        for (unsigned pixel_x = 0; pixel_x < CHAR_BIT; ++pixel_x) {
                ByteBitset palette_index = 0;
                palette_index.set(3, low_color_bits.test(0));
                palette_index.set(2, low_color_bits.test(1));
                palette_index.set(1, second_tile_row.test(pixel_x));
                palette_index.set(0, first_tile_row.test(pixel_x));
                Byte const color = background_color(palette_index.to_ulong());
                screen[tile_x * 8u + pixel_x][tile_y * 8u + row_num] = color;
        }
}

Byte PPU::background_color(unsigned palette_index) const noexcept
{
        return vram_.read_byte(VRAM::background_palette_start + palette_index);
}

Byte PPU::sprite_color(unsigned palette_index) const noexcept
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
        for (unsigned i = oam_address_; i < oam_address_ + oam_size; ++i) {
                unsigned const j = i % byte_max;
                unsigned const source_address = source * oam_size + j;
                oam_[j] = dma_memory_.read_byte(source_address);
        }
}

}

