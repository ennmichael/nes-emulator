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

namespace Sdl {

using Keycode  = SDL_Keycode;
using Color    = SDL_Color;
using Window   = SDL_Window;
using Renderer = SDL_Renderer;
using Texture  = SDL_Texture;

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

using Ticks = Uint32;
using Rect  = SDL_Rect;
using Rects = std::vector<Rect>;
using Event = SDL_Event;
using OptionalEvent = std::optional<Event>;

class Error : public std::exception {
public:
        char const* what() const noexcept override;
};

// Calls SDL_Init and SDL_Quit automatically.
class Initializer {
public:
        Initializer();
        ~Initializer();

        Initializer(Initializer const&) = delete;
        Initializer(Initializer&&) = delete;
        
        Initializer& operator=(Initializer const&) = delete;
        Initializer& operator=(Initializer&&) = delete;
};

// Sets a new render draw color in the constructor,
// resets to the old one in the destructor.
class RendererColorGuard {
public:
        RendererColorGuard(Renderer& Renderer, Color color);

        RendererColorGuard(RendererColorLock const&) = delete;
        RendererColorGuard(RendererColorLock&&) = delete;

        RendererColorGuard& operator=(RendererColorLock const&) = delete;
        RendererColorGuard& operator=(RendererColorLock&&) = delete;

        ~RendererColorGuard();

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

enum class Flip {
        none = SDL_FLIP_NONE,
        vertical = SDL_FLIP_VERTICAL,
        horizontal = SDL_FLIP_HORIZONTAL
};

// Uses current render color.
void render_filled_rect(Renderer& renderer, Rect rect);

// Uses specified color.
void render_filled_rect(Renderer& renderer, Rect rect, Color color);

void render_copy(Renderer& renderer,
                 Texture& texture,
                 Rect source,
                 Rect destination,
                 Flip flip=Flip::none,
                 double angle=0.);

Ticks get_ticks() noexcept;
OptionalEvent poll_event();

}

