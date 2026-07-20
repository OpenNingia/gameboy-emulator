#pragma once

#include <string>

#include <SDL2/SDL.h>

namespace gbemu {
    struct core;
    struct config;
    struct debugger;
    struct user_state;
    namespace gfx {
        struct backend;
    }
    namespace input {
        class manager;
    }
} // namespace gbemu

namespace gbemu::ui {

    // Opaque per-application UI state.  Owns the ImGui context and panel
    // visibility flags.  One instance per Application::run; not thread-safe.
    struct context;

    // Cross-boundary "requests" from the UI back to the hosting Application.
    // The UI sets fields here from menu-bar callbacks (which run inside
    // ImGui::NewFrame / Render); Application drains them *after* the current
    // frame has been presented so that blocking calls (native file dialog,
    // fullscreen toggle, quit) don't starve the GPU mid-frame.
    struct host_actions {
        // File -> Load ROM... clicked. Application opens its native file
        // dialog; if the user picks a file, the result is parked in
        // `pending_rom_load`.
        bool load_rom_dialog_requested{false};
        // Concrete ROM path to load (set by file dialog OR by a Recent ROMs
        // click). Application replaces the cartridge + resets the core and
        // clears the string.
        std::string pending_rom_load{};
        // File -> Exit clicked (or any other path the UI wants to trigger
        // a clean shutdown).
        bool quit_requested{false};
        // The UI has mutated `user_state` and wants the host to persist
        // it.  Set by add_recent_rom (and future palette / panel-state
        // mutations); drained by Application after each frame, which
        // calls user_state::save(user_conf_path_).  A single boolean is
        // enough — multiple mutations within one frame collapse into one
        // disk write.
        bool save_user_state_requested{false};
    };

    // Initialise ImGui (docking branch) bound to the given SDL2 window +
    // renderer, plus a gfx backend used by the UI to spawn presenters for
    // the GB display and the PPU tile/map viewers.  The SDL_Renderer is
    // still required for the imgui_impl_sdlrenderer2 backend; the gfx
    // backend hides the underlying texture API from the rest of ui.cpp.
    // Returns a handle to be passed to subsequent calls; the caller owns
    // lifetime and must invoke shutdown() before destroying the SDL
    // renderer/window/gfx backend.
    context* init(SDL_Window* window, SDL_Renderer* renderer, gfx::backend* backend, debugger& dbg, core& c,
                  user_state& user, input::manager const& input, std::string const& imgui_ini_path);

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

    // Access the cross-boundary action flags so Application can drain them
    // after each frame.  Returns a stable reference for the lifetime of the
    // context.
    host_actions& actions(context* ctx);

    // Push `path` onto the Recent ROMs MRU list (held in user_state).
    // Dedup, cap at 8 entries, then raise host_actions::save_user_state_requested
    // so the host writes user.conf to disk on this frame's drain pass.
    // Called by Application after a successful ROM load.
    void add_recent_rom(context* ctx, const std::string& path);

    // Install the built-in palettes, scan `palettes_dir` for user *.sbp
    // files, then apply `cfg.display.frame_blending` and `cfg.display.palette`
    // to the live blender and PPU resolver.  Unknown palette names fall
    // back to "grey" with a log warning.  Called by Application once after
    // ui::init.
    void apply_display_config(context* ctx, const config& cfg, const std::string& palettes_dir);

    // Clear the frame blender's history so the next frame shown is not
    // mixed with anything from the prior session.  Called from Application
    // after `core::reset()` (Ctrl+R or ROM hot-swap) so a freshly started
    // ROM doesn't ghost the prior image for a few frames.
    void reset_display_post(context* ctx);

} // namespace gbemu::ui
