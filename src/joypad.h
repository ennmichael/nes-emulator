#pragma once

#include "utils.h"
#include "sdl++.h"
#include <unordered_map>

// TODO Have an Sdl::Keyboard class so I can test this

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
        bool address_is_writable(Address address) const noexcept override;
        bool address_is_readable(Address address) const noexcept override;
        void write_byte(Address address, Byte byte) override;
        Byte read_byte(Address address) override;

private:
        bool strobe(Byte byte) const noexcept;
        bool button_is_down(JoypadButton button) const;

        Sdl::KeyboardState keyboard_state_;
        KeyBindings key_bindings_;
        Byte last_write_ = 0;
        int num_reads_ = 0;
};

}

