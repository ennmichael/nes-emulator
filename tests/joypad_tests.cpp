#include <cstdint>
#include "catch.hpp"
#include "../src/joypad.h"
#include "../src/sdl++.h"

TEST_CASE("Joypads test")
{
        std::array<std::uint8_t, 256> keyboard_state {};
        Emulator::KeyBindings const key_bindings {
                {Emulator::JoypadButton::b, Sdl::Scancode::a},
                {Emulator::JoypadButton::a, Sdl::Scancode::s},
                {Emulator::JoypadButton::select, Sdl::Scancode::enter},
                {Emulator::JoypadButton::start, Sdl::Scancode::spacebar},
                {Emulator::JoypadButton::up, Sdl::Scancode::up},
                {Emulator::JoypadButton::down, Sdl::Scancode::down},
                {Emulator::JoypadButton::left, Sdl::Scancode::left},
                {Emulator::JoypadButton::right, Sdl::Scancode::right}
        };
        Emulator::Joypad joypad(keyboard_state.data(), key_bindings);

        auto const set_keys = [&](std::vector<Sdl::Scancode> const& scancodes)
        {
                for (auto const scancode : scancodes)
                        keyboard_state[static_cast<std::size_t>(scancode)] = 1;
        };

        auto const check_single_key = [&](Sdl::Scancode scancode, int expected_read)
        {
                set_keys({scancode});
                for (int _ = 0; _ < 2; ++_) {
                        for (int i = 0; i < Emulator::Joypad::max_reads; ++i)
                        {
                                auto const byte = joypad.read_byte(Emulator::Joypad::first_joypad_address);
                                if (i == expected_read || i == 19)
                                        CHECK(byte == 1);
                                else
                                        CHECK(byte == 0);
                        }
                }
        };

	SECTION("Joypad A is pressed")
	{ check_single_key(Sdl::Scancode::s, 0); }

	SECTION("Joypad B is pressed")
	{ check_single_key(Sdl::Scancode::a, 1); }

        SECTION("Joypad SELECT is pressed")
        { check_single_key(Sdl::Scancode::enter, 2); }

        SECTION("Joypad START is pressed")
        { check_single_key(Sdl::Scancode::spacebar, 3); }

        SECTION("Joypad UP is pressed")
        { check_single_key(Sdl::Scancode::up, 4); }

        SECTION("Joypad DOWN is pressed")
        { check_single_key(Sdl::Scancode::down, 5); }

        SECTION("Joypad LEFT is pressed")
        { check_single_key(Sdl::Scancode::left, 6); }
        
        SECTION("Joypad RIGHT is pressed")
        { check_single_key(Sdl::Scancode::right, 7); }
}

