#pragma once

#include <string>
#include <vector>

#include <input.h>

namespace gbemu {

    // Per-session user state shared across the UI and the host Application.
    // Persisted as `<base>/<user_dir>/user.conf` (libconfig) — see TODO §14
    // for the schema.  Centralises what used to be three CWD-relative files
    // (`gbemu_window.state`, `gbemu_recent.txt`, the planned palette
    // preference) into a single portable-layout-aware blob.
    //
    // Default-constructed values are sentinel "absent" markers — the caller
    // (Application / UI) decides what to substitute when a field is missing
    // (e.g. DEFAULT_WINDOW_WIDTH when `window_w == 0`).  `active_palette`
    // is populated by future palette-switcher work; it's accepted in the
    // schema today so existing user.conf files survive that change without
    // a migration step.
    struct user_state {
        int window_w{0};
        int window_h{0};
        std::vector<std::string> recent_roms{};
        std::string active_palette{};

        // Audio output preferences. Defaults match "no menu interaction":
        // not muted, unity gain, accurate DC-blocking filter. Stored under
        // an `audio` sub-group in user.conf.
        bool audio_muted{false};
        float audio_volume{1.0f};
        // Serialised as "off" / "accurate" / "preserve". Unknown values
        // fall back to "accurate" on load — the typo path mirrors what
        // ui::apply_display_config does for unknown palettes.
        std::string audio_highpass{"accurate"};

        // Emulation speed multiplier (the persisted "speed slot" the user
        // picked from Emulation -> Speed or the +/-/0 hotkeys).  Stored
        // under an `emulation` sub-group; out-of-range values on load are
        // clamped to the [0.25, 4.0] preset window by Application before
        // being applied to the main loop budget.  Fast-forward (Tab hold)
        // is transient — see Application::fast_forward_active_ — and
        // deliberately does NOT update this field, so a crash mid-FF does
        // not bake the setting into user.conf.
        float speed_multiplier{1.0f};

        // Input bindings (hotkeys + joypad mapping).  Initialised to the
        // hard-coded `defaults()` so a missing `input` sub-block in
        // user.conf produces the same behaviour as the pre-step-3 builds.
        // load() replaces sub-arrays selectively (see src/user_state.cpp):
        // the `hotkeys` list, the keyboard half of `joypad`, and the
        // controller half each survive independently of one another so a
        // hand-edited user.conf that overrides only joypad_keyboard keeps
        // the default hotkeys intact.
        input::config input_bindings{input::config::defaults()};

        // Read libconfig from `path`.  Missing file (first run) or parse
        // error -> fields stay at construction defaults and returns false.
        // Never throws.
        bool load(const std::string& path);

        // Write current state to `path` via libconfig.  Atomic via
        // `.tmp` + `std::filesystem::rename`.  Best-effort: logs on
        // failure and returns false rather than throwing so a user.conf
        // glitch never brings the emulator down.
        bool save(const std::string& path) const;
    };

} // namespace gbemu
