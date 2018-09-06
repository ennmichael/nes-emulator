#pragma once

#include "SDL2/SDL.h"
#include <memory>
#include <unordered_map>
#include <utility>
#include <string>
#include <complex>
#include <exception>
#include <type_traits>
#include <vector>
#include <optional>
#include <cstdint>

namespace Sdl {

class Error : public std::exception {
public:
        char const* what() const noexcept override;
};

using Color = SDL_Color;
using Window = SDL_Window;
using Renderer = SDL_Renderer;
using Rect = SDL_Rect;
using Event = SDL_Event;
using KeyboardEvent = SDL_KeyboardEvent;
using OptionalEvent = std::optional<Event>;
using KeyboardState = std::uint8_t const*;

Color constexpr black {.r = 0, .g = 0, .b = 0, .a = 255};

enum class Scancode {
        left = SDL_SCANCODE_LEFT,
        right = SDL_SCANCODE_RIGHT,
        up = SDL_SCANCODE_UP,
        down = SDL_SCANCODE_DOWN,
        a = SDL_SCANCODE_A,
        s = SDL_SCANCODE_S,
        enter = SDL_SCANCODE_RETURN,
        spacebar = SDL_SCANCODE_SPACE
};

struct WindowDeleter {
        void operator()(Window* window) const noexcept;
};

struct RendererDeleter {
        void operator()(Renderer* Renderer) const noexcept;
};

using UniqueWindow   = std::unique_ptr<Window, WindowDeleter>;
using UniqueRenderer = std::unique_ptr<Renderer, RendererDeleter>;

class InitGuard {
public:
        InitGuard();
        ~InitGuard();
        InitGuard(InitGuard const&) = delete;
        InitGuard(InitGuard&&) = delete;
        InitGuard& operator=(InitGuard const&) = delete;
        InitGuard& operator=(InitGuard&&) = delete;
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

struct Context {
        UniqueWindow window;
        UniqueRenderer renderer;
};

UniqueWindow create_window(std::string const& title, int width, int height);
UniqueRenderer create_renderer(Window& window, Color background_color=black);
Context create_context(std::string const& title,
                       int width, int height,
                       Color background_color=black);

KeyboardState get_keyboard_state() noexcept;
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

OptionalEvent poll_event();

}

