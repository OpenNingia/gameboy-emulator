#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <cfg.h>
#include <user_state.h>

namespace gbemu {
    struct core;
}

class Application {
public:
    Application();
    ~Application();

    void set_rom_file(std::string_view path);
    void set_headless(bool h) { headless_ = h; }
    void set_script_path(std::string_view p) { script_path_ = p; }
    void set_output_path(std::string_view p) { output_path_ = p; }

    void run();

private:
    // Load a ROM file, compute its FNV1a hash (used as the .sav key),
    // hand it to the core, and pull in any matching battery save from
    // <base>/<savs_dir>/<hash>.sav.  Sets current_rom_hash_ on success.
    // Used both for the initial CLI-provided ROM and for the in-app
    // File → Load ROM hot-swap path.
    void load_rom_(gbemu::core& core, const std::string& path);

    // Persist the current cartridge's SRAM (and RTC, if it carries one)
    // to <base>/<savs_dir>/<current_rom_hash>.sav using the BESS format.
    // No-op when there is no cart attached, no battery, or no hash.
    // Writes are atomic (.sav.tmp → rename).
    void flush_battery_save_(gbemu::core& core);

    // Resolved at construction time from $GBEMU_HOME / SDL_GetBasePath
    // — see gbemu::paths::resolve_base_dir. Anchors all data paths
    // (cfg, bios, roms, savs, sslots).
    std::string base_;
    gbemu::config cfg;
    // Empty until set_rom_file() runs. When empty at run() time the core
    // is left without a cartridge (MMU returns OPEN_BUS for cart reads)
    // and the emulator opens in a paused state.
    std::string rom_path_{};
    bool headless_{false};
    std::string script_path_{};
    std::string output_path_{};

    // FNV1a hash of the currently loaded ROM (16-char lowercase hex),
    // used to derive the .sav filename.  Empty when no ROM is loaded;
    // re-derived on every load_rom_() including the in-app hot-swap.
    std::string current_rom_hash_{};
    // Wall-clock time of the last successful battery flush, in
    // milliseconds since SDL init.  Drives the periodic flush gate so
    // we don't write the .sav on every frame.  Reset to 0 when a new
    // ROM is loaded so the first dirty period after a swap flushes
    // promptly.
    std::uint64_t last_battery_flush_ms_{0};

    // Session state persisted under <base>/<user_dir>/user.conf.
    // Loaded at the top of run(), saved at shutdown (window pose) and
    // on-demand when the UI flags `save_user_state_requested` (e.g.
    // after a Recent ROMs mutation).  `user_conf_path_` is resolved
    // once in run() and reused for both load and save sites.
    gbemu::user_state user_state_{};
    std::string user_conf_path_{};
};
