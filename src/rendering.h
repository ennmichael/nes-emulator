// vim: set shiftwidth=8 tabstop=8:

#pragma once

#include <stdexcept>
#include "sdl++.h"
#include "ppu.h"
#include "utils.h"

namespace Emulator {

class UnknownColor : public std::runtime_error {
public:
        explicit UnknownColor(Byte nes_color) noexcept;
};

void render_screen(Sdl::Renderer& renderer, Screen const& screen);
Sdl::Color nes_color_to_rgb(Byte nes_color);

}

