#include "sdl++.h"

namespace Sdl {

void WindowDeleter::operator()(Window* window) const noexcept
{
        SDL_DestroyWindow(window);
}

void RendererDeleter::operator()(Renderer* Renderer) const noexcept
{
        SDL_DestroyRenderer(Renderer);
}

void TextureDeleter::operator()(Texture* texture) const noexcept
{
        SDL_DestroyTexture(texture);
}

char const* Error::what() const noexcept
{
        return SDL_GetError();
}

Initializer::Initializer()
{
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
                throw Error();
}

Initializer::~Initializer()
{
        SDL_Quit();
}

RendererColorGuard::RendererColorLock(Renderer& renderer, Color color)
        : renderer_(renderer)
        , previous_color_(get_render_color(renderer))
{
        set_render_color(renderer_, color);
}

RendererColorGuard::~RendererColorLock()
{
        set_render_color(renderer_, previous_color_);
}

UniqueWindow create_window(std::string const& title, int width, int height)
{
        auto* window = SDL_CreateWindow(title.c_str(),
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        width,
                                        height,
                                        0);

        if (!window)
                throw Error();

        return UniqueWindow(window);
}

namespace {

void set_blend_mode(Renderer& renderer)
{
        SDL_SetRenderDrawBlendMode(&renderer, SDL_BLENDMODE_BLEND);
}

}

UniqueRenderer create_renderer(Window& window, Color color)
{
        auto* renderer = SDL_CreateRenderer(&window, -1, 0);

        if (!renderer)
                throw Error();

        set_render_color(*renderer, color);
        set_blend_mode(*renderer);
        return UniqueRenderer(renderer);
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

void render_copy(Renderer& renderer, 
                 Texture& texture, 
                 Rect source, 
                 Rect destination,
                 Flip flip,
                 double angle)
{
        if (SDL_RenderCopyEx(&renderer,
                             &texture,
                             &source,
                             &destination,
                             angle,
                             nullptr, // Center point, fine this way.
                             static_cast<SDL_RendererFlip>(flip)) < 0) {
                throw Error();
        }
}

Ticks get_ticks() noexcept
{
        return SDL_GetTicks();
}


OptionalEvent poll_event()
{
        Event event;
        if (SDL_PollEvent(&event))
                return event;
        return std::nullopt;
}

}

