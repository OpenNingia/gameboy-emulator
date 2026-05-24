#pragma once

#include <SDL2/SDL.h>

namespace gbemu {
    struct core;
    struct debugger;
    namespace gfx {
        struct backend;
    }
} // namespace gbemu

namespace gbemu::ui {

    // Opaque per-application UI state.  Owns the ImGui context and panel
    // visibility flags.  One instance per Application::run; not thread-safe.
    struct context;

    // Initialise ImGui (docking branch) bound to the given SDL2 window +
    // renderer, plus a gfx backend used by the UI to spawn presenters for
    // the GB display and the PPU tile/map viewers.  The SDL_Renderer is
    // still required for the imgui_impl_sdlrenderer2 backend; the gfx
    // backend hides the underlying texture API from the rest of ui.cpp.
    // Returns a handle to be passed to subsequent calls; the caller owns
    // lifetime and must invoke shutdown() before destroying the SDL
    // renderer/window/gfx backend.
    context* init(SDL_Window* window, SDL_Renderer* renderer, gfx::backend* backend, debugger& dbg, core& c);

    // Tear down ImGui in reverse order.  Safe to call with nullptr.
    void shutdown(context* ctx);

    // Forward an SDL event to ImGui.  Returns true when ImGui consumed the
    // event (e.g. mouse over a panel, keyboard captured by a text field) so
    // the caller can suppress its own dispatch.
    bool process_event(context* ctx, const SDL_Event& e);

    // Build one ImGui frame: dockspace + menu bar + panels + render to the
    // bound SDL renderer.  The caller still owns SDL_RenderClear and
    // SDL_RenderPresent.  Consumes the PPU's frame-ready edge to refresh
    // the Display panel's texture in-place.
    void render_frame(context* ctx);

} // namespace gbemu::ui
