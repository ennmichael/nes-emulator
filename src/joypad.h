// vim: set shiftwidth=8 tabstop=8:

#pragma once

#include "utils.h"
#include "sdl++.h"
#include <unordered_map>

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

class Joypad {
public:
        static int constexpr max_reads = 24;

        Joypad(Sdl::KeyboardState keyboard_state,
               KeyBindings key_bindings,
               int signature) noexcept;

        void write_byte(Byte byte) noexcept;
        Byte read_byte() noexcept;

private:
        bool strobe(Byte byte) const noexcept;
        bool button_is_down(JoypadButton button) const;

        Sdl::KeyboardState keyboard_state_;
        KeyBindings key_bindings_;
        int signature_;
        Byte last_write_ = 0;
        int num_reads_ = 0;
};

int constexpr first_joypad_signature = 19;

class JoypadMemory : public Memory {
public:
        static Address constexpr first_joypad_address = 0x4016;

        JoypadMemory(Sdl::KeyboardState keyboard_state,
                     KeyBindings first_joypad_key_bindings) noexcept;

protected:
        bool address_is_writable_impl(Address address) const noexcept override;
        bool address_is_readable_impl(Address address) const noexcept override;
        void write_byte_impl(Address address, Byte byte) override;
        Byte read_byte_impl(Address address) override;

private:
        Joypad first_joypad_;
        // Possibly a second_joypad_ in the future
};

}

