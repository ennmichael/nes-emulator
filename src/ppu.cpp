#include "ppu.h"

namespace Emulator {

namespace {

template <class F>
void for_each_sprite(OAM const& oam, F const& f)
{
        for (auto i = oam.cbegin(); i != oam.cend(); i += 4) {
                f(Sprite{
                        .x = *(i + 3),
                        .y = *i,
                        .tile_index = *(i + 1),
                        .attributes = *(i + 2)
                });
        }
}

}

bool VRAM::address_is_valid(unsigned address) noexcept
{
        return address < size;
}

void VRAM::write_byte(unsigned address, Byte byte)
{
        if (!address_is_valid(address)) {
                throw InvalidAddress("Can't write to VRAM address"s +
                                     Utils::format_address(address));
        }

        destination(*this, address) = byte;
}

void VRAM::read_byte(unsigned address) const
{
        if (!address_is_valid(address)) {
                throw InvalidAddress("Can't read VRAM address"s +
                                     Utils::format_address(address));
        }

        return destination(*this, address);
}

template <class Self>
static auto& VRAM::destination(Self& self, unsigned address) noexcept
{
        if (pattern_tables_start <= address && address < pattern_tables_end) {
                return pattern_tables_[address];
        } else if (name_tables_start <= address && address < name_tables_end) {
                return name_tables_[address % name_tables_real_size_];
        } else if (palettes_start <= address && address < palettes_end) {
                return palettes_[address % palettes_real_size_];
        } else {
                return destination(self, address % palettes_end);
        }
}

auto Sprite::priority() const noexcept -> Priority
{
        return (attributes.test(5)) ?
                Priority::above_background : Priority::beneath_background;
}

bool Sprite::flip_vertically() const noexcept
{
        return attibutes.test(7);
}

bool Sprite::flip_horizontally() const noexcept
{
        return attibutes.test(6);
}

ByteBitset Sprite::color_bits() const noexcept
{
        return attributes & 0xC0u;
}

void DoubleWriteRegister::write_half(Byte byte) noexcept
{
        if (complete_)
                low_byte_ = byte;
        else
                high_byte = byte;
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
        return Utils::combine_bytes(low_byte_, high_byte_);
}

Byte DoubleWriteRegister::read_low_byte() const noexcept
{
        return low_byte_;
}

Byte DoubleWriteRegister::read_high_byte() const noexcept
{
        return high_byte_;
}

bool DoubleWriteRegister::complete() const noexcept
{
        return complete_;
}

PPU::PPU(Memory const& dma_memory) noexcept
        : dma_memory_(dma_memory)
{}

void PPU::vblank_started()
{}

void PPU::vblank_finished()
{}

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
                        oam_address_.write_half(byte);
                        break;

                case oam_data_register:
                        write_oam(oam_address_.read_whole(), byte);
                        break;

                case scroll_register:
                        scroll_.write_half(byte);
                        break;

                case vram_address_register:
                        vram_address_.write_half(byte);
                        break;

                case vram_data_register:
                        write_vram(vram_address_.read_whole(), byte);
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

Byte PPU::read_byte(unsigned address) const
{
        switch (address) {
                case control_register:
                        throw_not_readable("control"s, control_register);

                case mask_register:
                        throw_not_readable("mask"s, mask_register);

                case status_register:
                        return status_;

                case oam_address_register:
                        throw_not_readable("OAM address"s, oam_address_register);

                case oam_data_register:
                        {
                                unsigned const address = oam_address_.read_whole();
                                Byte const result = read_oam(address);
                                increment_oam_address();
                                return result;
                        }

                case scroll_register:
                        throw_not_readable("scroll"s, scroll_register);

                case vram_address_register:
                        throw_not_readable("VRAM address"s, vream_address_register);

                case vram_data_register:
                        {
                                unsigned const address = vram_address_.read_whole();
                                Byte const result = read_vram(address);
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
        throw InvalidAccess("PPU "s + register_name + " register ("s +
                            Utils::format_hex(address, 4) + ") is not writable."s);
}

void PPU::throw_not_readable(std::string const& register_name,
                             unsigned address)
{
        throw InvalidAccess("PPU "s + register_name + " register ("s +
                            Utils::format_hex(address, 4) + ") is not readable."s);
}

void PPU::throw_not_valid(unsigned address)
{
        throw InvalidAccess(Utils::format_hex(address, 4) +
                            " is not a valid PPU memory address."s);
}

unsigned PPU::base_name_table_address() const noexcept
{
        unsigned const mult = (control_ & 0xC0u).to_ulong();
        unsigned const offset =
                mult * (VRAM::name_table_size + VRAM::attirbute_table_size);
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

void PPU::increment_oam_address() noexcept
{
        oam_address_ += address_increment_offset();
}

void PPU::increment_vram_address() noexcept
{
        vram_address_.increment(address_increment_offset());
}

void execute_dma(Byte source)
{
        for (unsigned i = oam_address_; i < oam_address_ + oam_size_; ++i) {
                unsigned const j = i % byte_max;
                unsigned const source_address = source * oam_size + j;
                oam_.at(j) = dma_memory_.read_byte(source_address);
        }
}

}

