#pragma once

#include <string>
#include <vector>

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
