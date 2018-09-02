#include "sdl++.h"
#include "cpu.h"
#include "cartridge.h"
#include "ppu.h"
#include "joypads.h"
#include <iostream>
#include <utility>

using namespace std::string_literals;

namespace {

// FIXME Move this function out of here
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

        auto cartridge = Emulator::Cartridge::make(argv[1]);
        Emulator::Joypads joypads;
        auto ram = std::make_unique<Emulator::CPU::RAM>();
        auto ppu = std::make_unique<Emulator::PPU>(*ram);
        auto cpu = std::make_unique<Emulator::CPU>(
                Emulator::CPU::AccessibleMemory::Pieces{ram.get(), ppu.get(),
                                                        cartridge.get(), &joypads});

        std::cout << std::hex << "0x" << (unsigned)cpu->a() << '\n';
        std::cout << cpu->p() << '\n';
}

