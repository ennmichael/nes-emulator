// vim: set shiftwidth=8 tabstop=8:

#pragma once

#include "sdl++.h"
#include "ppu.h"

namespace Emulator {

void render_screen(Sdl::Renderer& renderer, Screen const& screen);
void render_pixel(Sdl::Renderer& renderer, Sdl::Color color, int x, int y);
Sdl::Color nes_color_to_rgb(Byte nes_color) noexcept;

}

