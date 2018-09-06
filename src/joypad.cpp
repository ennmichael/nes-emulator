#include <cassert>
#include <utility>
#include "joypad.h"
#include "sdl++.h"

using namespace std::string_literals;

namespace Emulator {

Joypad::Joypad(Sdl::KeyboardState keyboard_state, KeyBindings key_bindings) noexcept
        : keyboard_state_(keyboard_state)
        , key_bindings_(std::move(key_bindings))
{}

bool Joypad::address_is_writable(Address address) const noexcept
{
        return address == first_joypad_address;
}

bool Joypad::address_is_readable(Address address) const noexcept
{
        return address == first_joypad_address;
}

void Joypad::write_byte(Address address, Byte byte)
{
        if (!address_is_writable(address)) {
                throw InvalidAddress("Can't write to Joypad address "s +
                                     Utils::format_address(address));
        }
        if (strobe(byte))
                num_reads_ = 0;
        last_write_ = byte;
}

Byte Joypad::read_byte(Address address)
{
        if (!address_is_readable(address)) {
                throw InvalidAddress("Can't read Joypad address "s +
                                     Utils::format_address(address));
        }
        if (num_reads_ == max_reads)
                num_reads_ = 0;
        bool const result = [&]
        {
                const auto first_joypad_signature = 19;
                switch (num_reads_) {
                        case 0: return button_is_down(JoypadButton::a);
                        case 1: return button_is_down(JoypadButton::b);
                        case 2: return button_is_down(JoypadButton::select);
                        case 3: return button_is_down(JoypadButton::start);
                        case 4: return button_is_down(JoypadButton::up);
                        case 5: return button_is_down(JoypadButton::down);
                        case 6: return button_is_down(JoypadButton::left);
                        case 7: return button_is_down(JoypadButton::right);
                        case first_joypad_signature: return true;
                        default: return false;
                }
        }();
        ++num_reads_;
        return static_cast<Byte>(result);
}

bool Joypad::strobe(Byte byte) const noexcept
{
        return last_write_ == 1 && byte == 0;
}

bool Joypad::button_is_down(JoypadButton button) const
{
        auto const scancode = key_bindings_.at(button);
        return keyboard_state_[static_cast<std::size_t>(scancode)];
}

}

