#include "joypads.h"
#include "sdl++.h"

using namespace std::string_literals;

namespace Emulator {

bool Joypads::address_is_writable(unsigned address) const noexcept
{
        return address == first_joypad_address;
}

bool Joypads::address_is_readable(unsigned address) const noexcept
{
        return address == first_joypad_address || address == second_joypad_address;
}

void Joypads::write_byte(unsigned address, Byte byte)
{
        if (!address_is_writable(address)) {
                throw InvalidAddress("Can't write to Joypads address "s +
                                     Utils::format_address(address));
        }

        last_write_ = byte;
}

Byte Joypads::read_byte(unsigned address)
{
        if (address == first_joypad_address) {
                return read_first_joypad();
        } else if (address == second_joypad_address) {
                return 0;
        } else {
                throw InvalidAddress("Can't read Joypads address "s +
                                     Utils::format_address(address));
        }
}

bool Joypads::strobe() const noexcept
{
        return Utils::bit(last_write_, 0);
}

Byte Joypads::read_first_joypad() noexcept
{
        auto const a_is_down = []
        {
                return Sdl::key_is_down(Sdl::Scancode::y) ||
                       Sdl::key_is_down(Sdl::Scancode::z);
        };

        auto const b_is_down = []
        { return Sdl::key_is_down(Sdl::Scancode::x); };

        auto const select_is_down = []
        { return Sdl::key_is_down(Sdl::Scancode::a); };

        auto const start_is_down = []
        { return Sdl::key_is_down(Sdl::Scancode::s); };

        auto const up_is_down = []
        { return Sdl::key_is_down(Sdl::Scancode::up); };

        auto const down_is_down = []
        { return Sdl::key_is_down(Sdl::Scancode::down); };

        auto const left_is_down = []
        { return Sdl::key_is_down(Sdl::Scancode::left); };

        auto const right_is_down = []
        { return Sdl::key_is_down(Sdl::Scancode::right); };

        int const old_num_reads = num_reads_;
        ++num_reads_;
        if (strobe())
                num_reads_ = 0;
        switch (old_num_reads) {
                case 0: return a_is_down();
                case 1: return b_is_down();
                case 2: return select_is_down();
                case 3: return start_is_down();
                case 4: return up_is_down();
                case 5: return down_is_down();
                case 6: return left_is_down();
                case 7: return right_is_down();
        }
}

}

