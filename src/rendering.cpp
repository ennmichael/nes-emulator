#include "rendering.h"

namespace Emulator {

void render_screen(Sdl::Renderer& renderer, Screen const& screen)
{
        for (int y = 0; y < screen_height; ++y) {
                for (int x = 0; x < screen_width; ++x) {
                        auto const nes_color = screen[x][y];
                        auto const rgb_color = nes_color_to_rgb(nes_color);
                        render_pixel(renderer, rgb_color, x, y);
                }
        }
}

void render_pixel(Sdl::Renderer& renderer, Sdl::Color color, int x, int y)
{
        unsigned constexpr pixel_width = 2u;
        unsigned constexpr pixel_height = 2u;
        Rect const rect {
                .x = x,
                .y = y,
                .w = pixel_width,
                .h = pixel_height
        };

        render_filled_rect(renderer, rect, color);
}

}

