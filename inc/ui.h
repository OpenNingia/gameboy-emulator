#pragma once
#ifndef _H_UI_H_
#    define _H_UI_H_

#    include <SDL2/SDL.h>

namespace gbemu {
    struct core;
    struct debugger;
} // namespace gbemu

namespace gbemu::ui {

    // Opaque per-application UI state.  Owns the ImGui context and panel
    // visibility flags.  One instance per Application::run; not thread-safe.
    struct context;

    // Initialise ImGui (docking branch) bound to the given SDL2 window +
    // renderer.  Returns a handle to be passed to subsequent calls; the
    // caller owns lifetime and must invoke shutdown() before destroying
    // the SDL renderer/window.
    context* init(SDL_Window* window, SDL_Renderer* renderer, debugger& dbg, core& c);

    // Tear down ImGui in reverse order.  Safe to call with nullptr.
    void shutdown(context* ctx);

    // Forward an SDL event to ImGui.  Returns true when ImGui consumed the
    // event (e.g. mouse over a panel, keyboard captured by a text field) so
    // the caller can suppress its own dispatch.
    bool process_event(context* ctx, const SDL_Event& e);

    // Build one ImGui frame: dockspace + menu bar + panels + render to the
    // bound SDL renderer.  The caller still owns SDL_RenderClear and
    // SDL_RenderPresent.  `gb_texture` is the 160x144 ARGB streaming texture
    // shown in the Display panel.
    void render_frame(context* ctx, SDL_Texture* gb_texture);

} // namespace gbemu::ui

#endif // _H_UI_H_
