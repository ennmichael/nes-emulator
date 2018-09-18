// vim: set shiftwidth=8 tabstop=8:

#include "sdl++.h"
#include "cpu.h"
#include "cartridge.h"
#include "ppu.h"
#include "joypad.h"
#include <iostream>
#include <utility>

using namespace std::string_literals;

namespace {

void main_loop(Emulator::KeyBindings key_bindings)
{
        /**
         * A vblank occurs 60 times per second.
         * Inbetween vblanks, the emulator will execute at least 10 instructions.
         */
        Emulator::JoypadMemory joypad_memory(Sdl::get_keyboard_state(), key_bindings);
        Emulator::Cartridge cartridge(argv[1]);
        auto memory_mapper = Emulator::MemoryMapper::make(cartridge);
        auto ram = std::make_unique<Emulator::CPU::RAM>();
        auto ppu = std::make_unique<Emulator::PPU>(cartridge.mirroring(), *ram);
        auto cpu = std::make_unique<Emulator::CPU>(
                Emulator::CPU::AccessibleMemory::Pieces{ram.get(), ppu.get(),
                                                        memory_mapper.get(), &joypad_memory});
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

        Emulator::KeyBindings const key_bindings {  // Could read this from a config file if I wanted to
                {Emulator::JoypadButton::b, Sdl::Scancode::a},
                {Emulator::JoypadButton::a, Sdl::Scancode::s},
                {Emulator::JoypadButton::select, Sdl::Scancode::enter},
                {Emulator::JoypadButton::start, Sdl::Scancode::spacebar},
                {Emulator::JoypadButton::up, Sdl::Scancode::up},
                {Emulator::JoypadButton::down, Sdl::Scancode::down},
                {Emulator::JoypadButton::left, Sdl::Scancode::left},
                {Emulator::JoypadButton::right, Sdl::Scancode::right}
        };

        main_loop(key_bindings);
}

