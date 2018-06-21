#include "sdl++.h"
#include "cartridge.h"
#include <iostream>

using namespace std::string_literals;

int main(int argc, char** argv)
{
        Sdl::Initializer _;
        Emulator::Cartridge cartridge("../roms/NEStress.nes"s);
}
