// SDL_Renderer-backed implementation of the gfx::backend / gfx::presenter
// abstractions declared in inc/gfx.h.  Wraps SDL_CreateTexture +
// SDL_UpdateTexture; the public surface is intentionally tiny so the day a
// second backend (OpenGL3) lands it can live in its own translation unit
// with no impact on ui.cpp / app.cpp.

#include <SDL2/SDL.h>
#include <gfx.h>

namespace gbemu::gfx {

    struct backend {
        SDL_Renderer* renderer{nullptr};
    };

    struct presenter {
        SDL_Texture* texture{nullptr};
        int width{0};
        int height{0};
    };

    backend* sdl_backend_create(SDL_Renderer* r) {
        if (!r)
            return nullptr;
        auto* b = new backend{};
        b->renderer = r;
        return b;
    }

    void backend_destroy(backend* b) {
        delete b;
    }

    presenter* presenter_create(backend* b, int width, int height) {
        if (!b || !b->renderer || width <= 0 || height <= 0)
            return nullptr;
        SDL_Texture* tex =
            SDL_CreateTexture(b->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!tex)
            return nullptr;
        auto* p = new presenter{};
        p->texture = tex;
        p->width = width;
        p->height = height;
        return p;
    }

    void presenter_destroy(presenter* p) {
        if (!p)
            return;
        if (p->texture)
            SDL_DestroyTexture(p->texture);
        delete p;
    }

    void presenter_upload(presenter* p, const std::uint32_t* pixels) {
        if (!p || !p->texture || !pixels)
            return;
        SDL_UpdateTexture(p->texture, nullptr, pixels, p->width * 4);
    }

    ImTextureID presenter_imgui_id(const presenter* p) {
        if (!p || !p->texture)
            return ImTextureID{};
        return reinterpret_cast<ImTextureID>(p->texture);
    }

    int presenter_width(const presenter* p) {
        return p ? p->width : 0;
    }

    int presenter_height(const presenter* p) {
        return p ? p->height : 0;
    }

} // namespace gbemu::gfx
