#pragma once

#include <cstdint>

#include <imgui.h>

struct SDL_Renderer;

// Graphics backend abstraction.
//
// Hides the concrete "streaming ARGB texture that ImGui can draw" from the
// rest of the codebase so the UI does not depend on a specific renderer.
// Today there is exactly one backend (SDL_Renderer + SDL_Texture); the same
// API is meant to admit a future OpenGL3 backend (GLuint textures, post-FX
// shader passes for green-LCD / CRT effects) without touching ui.cpp.
//
// The two main types are opaque on purpose:
//
//   - backend   — bound to one underlying graphics device.  Created once by
//                 the Application; passed to ui::init so panels can spawn
//                 their own presenters off the same device.
//   - presenter — one streamable ARGB framebuffer, sized at creation time.
//                 One presenter per asset the UI wants to draw: the GB
//                 display (160x144), the VRAM tile viewer (128x192), the BG
//                 tile-map viewer (256x256), …
//
// Pixel format is always ARGB8888 with implicit stride = width * 4.

namespace gbemu::gfx {

    struct backend;
    struct presenter;

    // Construct a backend over the given SDL_Renderer.  Returns nullptr when
    // `r` is null.  The SDL_Renderer must out-live the backend.
    backend* sdl_backend_create(SDL_Renderer* r);

    // Destroy a backend.  Does not touch the underlying SDL_Renderer.  Safe
    // to call with nullptr.  All presenters created from this backend must
    // already have been destroyed.
    void backend_destroy(backend* b);

    // Allocate a presenter of the given pixel size on the backend's device.
    // Returns nullptr on failure (zero/negative size, allocation failure).
    presenter* presenter_create(backend* b, int width, int height);

    // Destroy a presenter, releasing its underlying texture.  Safe to call
    // with nullptr.
    void presenter_destroy(presenter* p);

    // Upload `presenter_width(p) * presenter_height(p)` ARGB8888 pixels into
    // the presenter.  Pixels must be tightly packed (stride = width * 4).
    void presenter_upload(presenter* p, const std::uint32_t* pixels);

    // ImGui texture handle suitable for ImGui::Image.  Stays valid until the
    // presenter is destroyed.  Returns a default-constructed handle (null)
    // when `p` is null.
    ImTextureID presenter_imgui_id(const presenter* p);

    int presenter_width(const presenter* p);
    int presenter_height(const presenter* p);

} // namespace gbemu::gfx
