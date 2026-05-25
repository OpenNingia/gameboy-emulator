#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <cfg.h>
#include <user_state.h>

struct SDL_Window;
struct SDL_Renderer;

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

    // Discrete speed presets surfaced by the Emulation -> Speed menu and
    // the +/- hotkeys. Kept in monotonically increasing order so
    // bump_speed_up_ / bump_speed_down_ can do a linear lookup.
    static constexpr float SPEED_PRESETS[] = {0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 4.0f};
    static constexpr int SPEED_PRESET_COUNT = sizeof(SPEED_PRESETS) / sizeof(float);

    // Bump user_state_.speed_multiplier to the next/previous preset (or
    // snap to 1.0x). Re-applies the effective audio mute and updates the
    // SDL window title via apply_speed_state_.
    void bump_speed_up_(gbemu::core& core);
    void bump_speed_down_(gbemu::core& core);
    void reset_speed_(gbemu::core& core);

    // Re-apply speed-derived side effects (window title, APU mute gate,
    // renderer vsync flip for fast-forward).  Called whenever
    // speed_multiplier_ or fast_forward_active_ flips, and at startup
    // once the window exists.  Also updates last_applied_speed_ so the
    // main loop's UI-mutation watcher stays consistent.
    void apply_speed_state_(gbemu::core& core);

    // Compute the effective APU mute = user_muted OR speed-driven mute
    // and push it into the APU.  Called from apply_speed_state_, the M
    // hotkey, and the save_user_state_requested drain pass.
    void apply_effective_mute_(gbemu::core& core);

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

    // Transient frame-pacing state.  `fast_forward_active_` is the Tab
    // key's hold state: while true the main loop swaps to a wall-clock
    // budgeted multi-frame burst with vsync disabled.  Not persisted —
    // a crash mid-FF must not leave user.conf in a "boot stuck at 8x"
    // state.  `vsync_disabled_` mirrors what we last set on the
    // renderer so we only call SDL_RenderSetVSync on edges.
    bool fast_forward_active_{false};
    bool vsync_disabled_{false};
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};

    // Last speed multiplier we pushed into apply_speed_state_.  Used by
    // the main loop to detect external mutations (UI menu writes to
    // user_state_ directly) so the title / apu mute / vsync stay
    // coherent without requiring every mutator to call a setter.
    float last_applied_speed_{1.0f};
};
