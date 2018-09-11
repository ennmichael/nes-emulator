// vim: set shiftwidth=8 tabstop=8:

#include <cassert>
#include <utility>
#include "joypad.h"
#include "sdl++.h"

using namespace std::string_literals;

namespace Emulator {

Joypad::Joypad(Sdl::KeyboardState keyboard_state, KeyBindings key_bindings, int signature) noexcept
        : keyboard_state_(keyboard_state)
        , key_bindings_(std::move(key_bindings))
        , signature_(signature)
{}

void Joypad::write_byte(Byte byte) noexcept
{
        if (strobe(byte))
                num_reads_ = 0;
        last_write_ = byte;
}

Byte Joypad::read_byte() noexcept
{
        if (num_reads_ == max_reads)
                num_reads_ = 0;
        bool const result = [&]
        {
                if (num_reads_ == signature_)
                        return true;
                switch (num_reads_) {
                        case 0: return button_is_down(JoypadButton::a);
                        case 1: return button_is_down(JoypadButton::b);
                        case 2: return button_is_down(JoypadButton::select);
                        case 3: return button_is_down(JoypadButton::start);
                        case 4: return button_is_down(JoypadButton::up);
                        case 5: return button_is_down(JoypadButton::down);
                        case 6: return button_is_down(JoypadButton::left);
                        case 7: return button_is_down(JoypadButton::right);
                }
                return false;
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

JoypadMemory::JoypadMemory(Sdl::KeyboardState keyboard_state, KeyBindings first_joypad_key_bindings) noexcept
        : first_joypad_(keyboard_state, first_joypad_key_bindings, first_joypad_signature)
{}

bool JoypadMemory::address_is_writable_impl(Address address) const noexcept
{
        return address == first_joypad_address;
}

bool JoypadMemory::address_is_readable_impl(Address address) const noexcept
{
        return address == first_joypad_address;
}

void JoypadMemory::write_byte_impl(Address, Byte byte)
{
        first_joypad_.write_byte(byte);
}

Byte JoypadMemory::read_byte_impl(Address)
{
        return first_joypad_.read_byte();
}

}

