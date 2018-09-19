// vim: set shiftwidth=8 tabstop=8:

#include "sdl++.h"
#include "cpu.h"
#include "cartridge.h"
#include "ppu.h"
#include "joypad.h"
#include "rendering.h"
#include <iostream>
#include <utility>

using namespace std::string_literals;

/*
 * TODO Fake the APU stuff - is there anything to fake?
 * FIXME Remove the main_loop function
 */

namespace {

unsigned constexpr vblanks_per_second = 60;
unsigned constexpr min_instructions_during_vblank = 10;
auto constexpr title = "";

int main_loop(int argc, char** argv)
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

        Emulator::Cartridge cartridge(argv[1]);
        Emulator::JoypadMemory joypad_memory(Sdl::get_keyboard_state(), key_bindings);
        auto memory_mapper = Emulator::MemoryMapper::make(cartridge);
        auto const ram = std::make_unique<Emulator::CPU::RAM>();
        auto const ppu = std::make_unique<Emulator::PPU>(cartridge.mirroring(), *ram);
        auto const cpu = std::make_unique<Emulator::CPU>(
                Emulator::CPU::AccessibleMemory::Pieces{ram.get(), ppu.get(),
                                                        memory_mapper.get(), &joypad_memory});

        Sdl::InitGuard init_guard;
        (void)init_guard;

        Sdl::Context const context = Sdl::create_context(title, Emulator::screen_width * 2, Emulator::screen_height * 2);

        /**
         * A vblank occurs vblanks_per_second times per second, also meaning
         * the screen is redrawn that many times each second.
         * During a vblank, the emulator will execute at least min_instructions_during_vblank instructions.
         */

        Sdl::Ticks const delay_ms = 1000 / vblanks_per_second;
        Sdl::Ticks last_vblank_ms = Sdl::get_ticks();
        unsigned instructions_executed = 0;
        unsigned debug_instructions_executed = 0; // TODO Delete this
        for (bool quit = false; !quit; quit = Sdl::quit_requested()) {
                if (instructions_executed >= min_instructions_during_vblank) {
                        auto const current_time_ms = Sdl::get_ticks();
                        if (current_time_ms - last_vblank_ms >= delay_ms) {
                                instructions_executed = 0;
                                last_vblank_ms = current_time_ms;
                                Sdl::render_clear(*context.renderer);
                                Emulator::render_screen(*context.renderer, ppu->current_screen());
                                Sdl::render_present(*context.renderer);
                        }
                }
                cpu->execute_instruction();
                ++instructions_executed;
                ++debug_instructions_executed;
                std::cout << debug_instructions_executed << '\n';
        }

        return 0;
}

}

int main(int argc, char** argv)
{
        return main_loop(argc, argv);
}

