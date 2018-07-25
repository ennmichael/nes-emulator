#include "cpu.h"
#include "cartridge.h"
#include "ppu.h"
#include <iostream>
#include <utility>

using namespace std::string_literals;

namespace {

std::pair<Emulator::UniqueCPU, Emulator::UniquePPU>
create_processing_units(Emulator::Cartridge& cartridge)
{
        return ProcessingChips {
                std::move(cpu),
                std::move(ppu)
        };
}

// FIXME Move this function out of here
void main_loop(CPU& cpu, PPU& ppu)
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

        [[maybe_unused]]
        Sdl::InitGuard init_guard;

        Emulator::Cartridge cartridge(argv[1]);
        Emulator::Joypads joypads;
        auto ram = std::make_unique<Emulator::CPU::RAM>();
        auto ppu = std::make_unique<Emulator::PPU>(*ram);
        auto cpu = std::make_unique<Emulator::CPU>({ram.get(), ppu.get(),
                                                    &cartridge, &joypads});

        std::cout << std::hex << "0x" << (unsigned)cpu.a << '\n';
        std::cout << cpu.p << '\n';
}

