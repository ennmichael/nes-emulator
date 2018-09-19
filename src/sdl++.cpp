// vim: set shiftwidth=8 tabstop=8:

#include "sdl++.h"

namespace Sdl {

char const* Error::what() const noexcept
{
        return SDL_GetError();
}

void WindowDeleter::operator()(Window* window) const noexcept
{
        SDL_DestroyWindow(window);
}

void RendererDeleter::operator()(Renderer* Renderer) const noexcept
{
        SDL_DestroyRenderer(Renderer);
}

InitGuard::InitGuard()
{
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
                throw Error();
}

InitGuard::~InitGuard()
{
        SDL_Quit();
}

RendererColorGuard::RendererColorGuard(Renderer& renderer, Color color)
        : renderer_(renderer)
        , previous_color_(get_render_color(renderer))
{
        set_render_color(renderer_, color);
}

RendererColorGuard::~RendererColorGuard()
{
        set_render_color(renderer_, previous_color_);
}

UniqueWindow create_window(std::string const& title, int width, int height)
{
        UniqueWindow window(SDL_CreateWindow(title.c_str(),
                                             SDL_WINDOWPOS_UNDEFINED,
                                             SDL_WINDOWPOS_UNDEFINED,
                                             width,
                                             height,
                                             0));
        if (!window)
                throw Error();
        return window;
}

namespace {

void set_blend_mode(Renderer& renderer)
{
        SDL_SetRenderDrawBlendMode(&renderer, SDL_BLENDMODE_BLEND);
}

}

UniqueRenderer create_renderer(Window& window, Color color)
{
        UniqueRenderer renderer(SDL_CreateRenderer(&window, -1, 0));

        if (!renderer)
                throw Error();

        set_render_color(*renderer, color);
        set_blend_mode(*renderer);
        return renderer;
}

Context create_context(std::string const& title,
                       int width, int height,
                       Color background_color)
{
        UniqueWindow window = create_window(title, width, height);
        UniqueRenderer renderer = create_renderer(*window, background_color);
        return {
                .window = std::move(window),
                .renderer = std::move(renderer)
        };
}

KeyboardState get_keyboard_state() noexcept
{
        return SDL_GetKeyboardState(nullptr);
}

void set_render_color(Renderer& renderer, Color color)
{
        SDL_SetRenderDrawColor(&renderer, color.r, color.g, color.b, color.a);
}

Color get_render_color(Renderer& Renderer)
{
        Color color;
        SDL_GetRenderDrawColor(&Renderer, &color.r, &color.g, &color.b, &color.a);
        return color;
}

void render_clear(Renderer& renderer)
{
        if (SDL_RenderClear(&renderer) < 0)
                throw Error();
}

void render_present(Renderer& renderer)
{
        SDL_RenderPresent(&renderer);
}

void render_filled_rect(Renderer& renderer, Rect rect)
{
        SDL_RenderFillRect(&renderer, &rect);
}

void render_filled_rect(Renderer& renderer, Rect rect, Color color)
{
        RendererColorGuard _(renderer, color);
        render_filled_rect(renderer, rect);
}

OptionalEvent poll_event()
{
        Event event;
        if (SDL_PollEvent(&event))
                return event;
        return std::nullopt;
}

Ticks get_ticks() noexcept
{
        return SDL_GetTicks();
}

bool quit_requested() noexcept
{
        return SDL_QuitRequested();
}

}

