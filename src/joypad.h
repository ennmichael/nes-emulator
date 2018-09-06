#pragma once

#include "utils.h"
#include "sdl++.h"
#include <unordered_map>

/* FIXME
 * This is a bit bad. We should have a Joypad class which isn't a type of
 * Memory, and a type of Memory called JoypadMemory which would also (some day?)
 * handle the second joypad as well. The Address parameter is unused so clearly that is bad.
 */

namespace Emulator {

enum class JoypadButton {
        a,
        b,
        select,
        start,
        up,
        down,
        left,
        right
};

using KeyBindings = std::unordered_map<JoypadButton, Sdl::Scancode>;

class Joypad : public Memory {
public:
        using Buttons = std::unordered_map<unsigned, bool>;

        static int constexpr max_reads = 24;
        static Byte constexpr base_joypad_value = 0x40;
        static Address constexpr first_joypad_address = 0x4016;

        Joypad(Sdl::KeyboardState keyboard_state, KeyBindings key_bindings) noexcept;
       
        void update();

protected:
        bool address_is_writable_impl(Address address) const noexcept override;
        bool address_is_readable_impl(Address address) const noexcept override;
        void write_byte_impl(Address address, Byte byte) override;
        Byte read_byte_impl(Address address) override;

private:
        bool strobe(Byte byte) const noexcept;
        bool button_is_down(JoypadButton button) const;

        Sdl::KeyboardState keyboard_state_;
        KeyBindings key_bindings_;
        Byte last_write_ = 0;
        int num_reads_ = 0;
};

}

