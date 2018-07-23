#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <utility>
#include <string>
#include <complex>
#include <exception>
#include <type_traits>
#include <vector>
#include <optional>

namespace Emulator::Sdl {

using Keycode = SDL_Keycode;
using Color = SDL_Color;
using Window = SDL_Window;
using Renderer = SDL_Renderer;
using Texture = SDL_Texture;
using Ticks = Uint32;
using Rect = SDL_Rect;
using Rects = std::vector<Rect>;
using Event = SDL_Event;
using OptionalEvent = std::optional<Event>;

namespace Colors {
        Color constexpr white {255, 255, 255, 255};
        Color constexpr black {0, 0, 0, 255};
}

namespace Keycodes {
        Keycode constexpr e = SDLK_e;
        Keycode constexpr a = SDLK_a;
        Keycode constexpr d = SDLK_d;
        Keycode constexpr w = SDLK_w;
        Keycode constexpr left = SDLK_LEFT;
        Keycode constexpr right = SDLK_RIGHT;
        Keycode constexpr up = SDLK_UP;
        Keycode constexpr space = SDLK_SPACE;
        Keycode constexpr left_shift = SDLK_LSHIFT;
}

struct WindowDeleter {
        void operator()(Window* window) const noexcept;
};

struct RendererDeleter {
        void operator()(Renderer* Renderer) const noexcept;
};

struct TextureDeleter {
        void operator()(Texture* texture) const noexcept;
};

using UniqueWindow   = std::unique_ptr<Window, WindowDeleter>;
using UniqueRenderer = std::unique_ptr<Renderer, RendererDeleter>;
using UniqueTexture  = std::unique_ptr<Texture, TextureDeleter>;

class Error : public std::exception {
public:
        char const* what() const noexcept override;
};

class Scope {
public:
        Scope();
        ~Scope();
        Scope(Scope const&) = delete;
        Scope(Scope&&) = delete;
        Scope& operator=(Scope const&) = delete;
        Scope& operator=(Scope&&) = delete;
};

class RendererColorGuard {
public:
        RendererColorGuard(Renderer& Renderer, Color color);
        ~RendererColorGuard();
        RendererColorGuard(RendererColorGuard const&) = delete;
        RendererColorGuard(RendererColorGuard&&) = delete;
        RendererColorGuard& operator=(RendererColorGuard const&) = delete;
        RendererColorGuard& operator=(RendererColorGuard&&) = delete;

private:
        Renderer& renderer_;
        Color previous_color_;
};

UniqueWindow create_window(std::string const& title, int width, int height);
UniqueRenderer create_renderer(Window& window, Color color=Colors::white);

void set_render_color(Renderer& Renderer, Color color);
Color get_render_color(Renderer& Renderer);
void render_clear(Renderer& Renderer);
void render_present(Renderer& Renderer);
void render_filled_rect(Renderer& renderer, Rect rect);
void render_filled_rect(Renderer& renderer, Rect rect, Color color);

enum class Flip {
        none = SDL_FLIP_NONE,
        vertical = SDL_FLIP_VERTICAL,
        horizontal = SDL_FLIP_HORIZONTAL
};

void render_copy(Renderer& renderer,
                 Texture& texture,
                 Rect source,
                 Rect destination,
                 Flip flip=Flip::none,
                 double angle=0.);

Ticks get_ticks() noexcept;
OptionalEvent poll_event();

}

