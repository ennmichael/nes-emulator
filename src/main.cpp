#include "sdl++.h"
#include "cpu.h"
#include "cartridge.h"
#include "ppu.h"
#include "joypad.h"
#include <iostream>
#include <utility>

using namespace std::string_literals;

namespace {

void main_loop(Emulator::CPU& cpu, Emulator::PPU& ppu)
{
        for (;;) {

        }
}

}

int main(int argc, char** argv)
{
        if (argc != 2) {
                std::cout << "Incorrect command-line arguments.\n";
                return 1;
        }

        Sdl::InitGuard init_guard;
        (void)init_guard;

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

        auto keyboard_state = Sdl::get_keyboard_state();
        Emulator::JoypadMemory joypad_memory(keyboard_state, key_bindings);
        auto cartridge = Emulator::Cartridge::make(argv[1]);
        auto ram = std::make_unique<Emulator::CPU::RAM>();
        auto ppu = std::make_unique<Emulator::PPU>(*ram);
        auto cpu = std::make_unique<Emulator::CPU>(
                Emulator::CPU::AccessibleMemory::Pieces{ram.get(), ppu.get(),
                                                        cartridge.get(), &joypad_memory});
}

