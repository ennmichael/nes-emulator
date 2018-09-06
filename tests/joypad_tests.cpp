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
                keyboard_state = {};
                for (auto const scancode : scancodes)
                        keyboard_state[static_cast<std::size_t>(scancode)] = 1;
        };

        auto const check_pressed = [&](std::vector<Sdl::Scancode> const& scancodes,
                                       std::vector<int> const& expected_reads)
        {
                set_keys(scancodes);
                for (int _ = 0; _ < 2; ++_) {
                        for (int i = 0; i < Emulator::Joypad::max_reads; ++i)
                        {
                                auto const byte = joypad.read_byte(Emulator::Joypad::first_joypad_address);
                                if (i == 19 || std::find(expected_reads.cbegin(), expected_reads.cend(), i) != expected_reads.cend())
                                        CHECK(byte == 1);
                                else
                                        CHECK(byte == 0);
                        }
                }
        };

        auto const check_pressed_and_released = [&](std::vector<Sdl::Scancode> const& scancodes,
                                                    std::vector<int> const& expected_reads)
        {
                check_pressed(scancodes, expected_reads);
                check_pressed({}, {});
                check_pressed(scancodes, expected_reads);
        };

        SECTION("A is pressed and released")
        { check_pressed_and_released({Sdl::Scancode::s}, {0}); }

        SECTION("B is pressed and released")
        { check_pressed_and_released({Sdl::Scancode::a}, {1}); }

        SECTION("SELECT is pressed and released")
        { check_pressed_and_released({Sdl::Scancode::enter}, {2}); }

        SECTION("START is pressed and released")
        { check_pressed_and_released({Sdl::Scancode::spacebar}, {3}); }

        SECTION("UP is pressed and released")
        { check_pressed_and_released({Sdl::Scancode::up}, {4}); }

        SECTION("DOWN is pressed and released")
        { check_pressed_and_released({Sdl::Scancode::down}, {5}); }

        SECTION("LEFT is pressed and released")
        { check_pressed_and_released({Sdl::Scancode::left}, {6}); }

        SECTION("RIGHT is pressed and released")
        { check_pressed_and_released({Sdl::Scancode::right}, {7}); }

        SECTION("A and B are pressed and released")
        {
                check_pressed_and_released({Sdl::Scancode::s, Sdl::Scancode::a}, {0, 1});
        }

        SECTION("A and B and UP are pressed and released")
        {
                check_pressed_and_released({Sdl::Scancode::s, Sdl::Scancode::a, Sdl::Scancode::up},
                                           {0, 1, 4});
        }

        SECTION("SELECT and START are pressed and released")
        {
                check_pressed_and_released({Sdl::Scancode::enter, Sdl::Scancode::spacebar},
                                           {2, 3});
        }

        SECTION("UP, DOWN, LEFT and RIGHT are pressed and released")
        {
                std::vector const scancodes {
                        Sdl::Scancode::up, Sdl::Scancode::down,
                        Sdl::Scancode::left, Sdl::Scancode::right
                };
                check_pressed_and_released(scancodes, {4, 5, 6, 7});
        }
}

