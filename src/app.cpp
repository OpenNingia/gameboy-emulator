#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include <SDL2/SDL.h>
#include <app.h>
#include <apu.h>
#include <audio_ring_buffer.hpp>
#include <bess.h>
#include <card.h>
#include <core.h>
#include <debugger.h>
#include <exc.hpp>
#include <gb_layout.h>
#include <gfx.h>
#include <input.h>
#include <joypad.h>
#include <log.h>
#include <mbc.h>
#include <mmu.h>
#include <paths.h>
#include <ui.h>

#ifdef _WIN32
#    include <SDL2/SDL_syswm.h>
#    include <windows.h>
// Pulled in after windows.h so MAX_PATH / OPENFILENAMEW are visible.
#    include <commdlg.h>
#    pragma comment(lib, "comdlg32.lib")
#endif

namespace gbemu {
    // Defined in src/script_runner.cpp.
    int run_script(debugger& dbg, const std::string& script_path, const std::string& out_path);
} // namespace gbemu

namespace {
    // Initial SDL window dimensions used the first time the app runs (no
    // user.conf on disk yet).  Subsequent launches restore the user's
    // last size from `user_state.window_w / window_h`.
    constexpr int DEFAULT_WINDOW_WIDTH = 1280;
    constexpr int DEFAULT_WINDOW_HEIGHT = 720;

    // SDL audio callback — invoked on the SDL audio thread when the device
    // wants more samples.  Drains stereo float frames out of the APU's ring
    // buffer; on underrun (ring empty) the remaining slots are filled with
    // silence so the audio device keeps consuming on schedule.
    void SDLCALL audio_callback(void* userdata, Uint8* stream, int len) {
        auto* ring = static_cast<gbemu::audio_ring_buffer*>(userdata);
        auto* out = reinterpret_cast<float*>(stream);
        const int frames = len / int(2 * sizeof(float));
        for (int i = 0; i < frames; ++i) {
            float l = 0.0f, r = 0.0f;
            ring->pop(l, r); // pop() leaves l/r at 0 on underrun
            out[i * 2] = l;
            out[i * 2 + 1] = r;
        }
    }

    // Open a native "Load ROM" file dialog parented to `parent`. Returns the
    // picked path (UTF-8) or "" on cancel.  The dialog runs its own message
    // pump, so the SDL window freezes for the duration — acceptable for a
    // user-initiated modal action.  OFN_NOCHANGEDIR prevents the dialog from
    // mutating the process CWD (we rely on it for imgui.ini / window state).
    std::string open_rom_dialog(SDL_Window* parent, const std::string& initial_dir) {
#ifdef _WIN32
        HWND hwnd = nullptr;
        SDL_SysWMinfo wmi;
        SDL_VERSION(&wmi.version);
        if (parent && SDL_GetWindowWMInfo(parent, &wmi))
            hwnd = wmi.info.win.window;

        // Convert initial_dir (UTF-8) to wide for the dialog, then flip any
        // forward slashes to backslashes — Win32 common dialogs silently
        // ignore lpstrInitialDir when it contains `/`, and the repo-wide
        // convention is to emit POSIX-style separators (see paths::to_generic).
        std::wstring wdir;
        if (!initial_dir.empty()) {
            const int len = MultiByteToWideChar(CP_UTF8, 0, initial_dir.c_str(), -1, nullptr, 0);
            if (len > 1) {
                wdir.resize(static_cast<std::size_t>(len - 1));
                MultiByteToWideChar(CP_UTF8, 0, initial_dir.c_str(), -1, wdir.data(), len);
                for (auto& ch : wdir)
                    if (ch == L'/')
                        ch = L'\\';
            }
        }

        wchar_t path[MAX_PATH] = {0};
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = L"Game Boy ROMs (*.gb;*.gbc)\0*.gb;*.gbc\0All files (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = L"Load ROM";
        ofn.lpstrInitialDir = wdir.empty() ? nullptr : wdir.c_str();
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        if (!GetOpenFileNameW(&ofn))
            return {};

        const int u8len = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
        if (u8len <= 1)
            return {};
        std::string result(static_cast<std::size_t>(u8len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, path, -1, result.data(), u8len, nullptr, nullptr);
        // Round-trip through filesystem::path so the returned UTF-8 picks up
        // forward-slash separators — repo-wide convention (see memory note
        // "Forward slashes everywhere").
        return std::filesystem::path{result}.generic_string();
#else
        (void)parent;
        (void)initial_dir;
        // Non-Windows builds: TODO when nfd / tinyfiledialogs lands in vcpkg.json.
        return {};
#endif
    }
} // namespace

Application::Application()
    : base_(gbemu::paths::resolve_base_dir()), cfg(gbemu::paths::resolve_under(base_, "cfg/gbemu.conf")) {
    LOG_INFO(gbemu::log::root(), "base dir: {}", base_);

    // SDL_INIT_GAMECONTROLLER pulls in SDL_INIT_JOYSTICK and SDL_INIT_EVENTS;
    // we rely on it for both the SDL_GameController API used below and the
    // automatic SDL_CONTROLLERDEVICEADDED events fired at startup for
    // already-connected pads.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
        throw gbemu::gbemu_exception{"SDL Initialization failed!"};
}

Application::~Application() {
    SDL_Quit();
}

static void load_bios(gbemu::core& core, const std::string& path) {
    gbemu::bios_file c{};
    c.load_from(path);
    core.load(c);
}

void Application::load_rom_(gbemu::core& core, const std::string& path) {
    gbemu::rom_file rf{};
    rf.load_from(path);

    // Hash the ROM bytes BEFORE core.load(): core.load() moves rf.data
    // into the freshly-built MBC, leaving rf.data empty.  The hash keys
    // the .sav filename and must be stable across renames of the ROM
    // file — FNV1a over the full image gives us that.
    const std::string hash = gbemu::paths::fnv1a_64_hex(rf.data);

    core.load(rf);
    current_rom_hash_ = hash;
    last_battery_flush_ms_ = 0;

    auto* cart = core.mmu.cart();
    if (!cart || !cart->has_battery())
        return;

    // Load the matching .sav, if any.  Missing file is normal (first
    // run on this cart); read errors are logged and skipped — we don't
    // want a half-baked file to bring the emulator down.
    const auto sav_path = gbemu::paths::resolve_data_path(base_, cfg.paths.savs_dir, hash + ".sav");
    std::ifstream f(sav_path, std::ios::binary);
    if (!f) {
        LOG_INFO(gbemu::log::root(), "battery: no .sav for {} (cart={:#04x})", hash, cart->debug_state().type);
        return;
    }
    std::vector<std::uint8_t> buf((std::istreambuf_iterator<char>(f)), {});
    f.close();

    auto parsed = gbemu::bess::parse_sav(buf);

    // SRAM restore.  Size mismatch (cart RAM has changed schema, or the
    // file came from a different cart that happened to share a hash —
    // shouldn't happen with FNV1a but a noisy log helps debugging) is
    // ignored at the MBC level; warn here so the user knows the load
    // was incomplete.
    const auto expected = cart->ram_data().size();
    if (!parsed.sram.empty()) {
        if (parsed.sram.size() == expected) {
            cart->ram_load(parsed.sram);
        } else {
            LOG_WARNING(gbemu::log::root(), "battery: .sav SRAM size {} bytes != cart {} bytes — ignored",
                        parsed.sram.size(), expected);
        }
    }

    if (parsed.rtc.has_value()) {
        cart->rtc_load_blob(*parsed.rtc);
        LOG_INFO(gbemu::log::root(), "battery: loaded {} SRAM bytes + RTC for {}", parsed.sram.size(), hash);
    } else {
        LOG_INFO(gbemu::log::root(), "battery: loaded {} SRAM bytes for {}", parsed.sram.size(), hash);
    }

    // The fresh RAM/RTC may have arrived as already-clean data; clear
    // the dirty bit so the periodic flush doesn't immediately rewrite
    // an identical file.
    cart->ram_clear_dirty();
}

void Application::flush_battery_save_(gbemu::core& core) {
    if (current_rom_hash_.empty())
        return;
    auto* cart = core.mmu.cart();
    if (!cart || !cart->has_battery())
        return;

    const auto sram = cart->ram_data();
    const auto rtc = cart->rtc_blob();

    // Nothing to serialize: bare BATTERY type with zero-sized RAM and
    // no RTC.  Theoretically possible (header byte misconfigured); just
    // skip to avoid emitting an empty BESS file.
    if (sram.empty() && !rtc.has_value())
        return;

    auto out = gbemu::bess::write_sav(sram, rtc, "GbEmu 0.1.0-dev");
    if (out.empty())
        return;

    const auto sav_path = gbemu::paths::resolve_data_path(base_, cfg.paths.savs_dir, current_rom_hash_ + ".sav");
    const auto tmp_path = sav_path + ".tmp";

    // Atomic write: dump to .tmp first, then rename over the real path.
    // std::filesystem::rename is atomic on the same filesystem on every
    // platform we ship to (Windows NTFS does a posix-style replace
    // since Win10), so a crash mid-write leaves either the previous
    // .sav intact or, at worst, an orphaned .tmp that we can ignore on
    // next boot.
    {
        std::ofstream f(tmp_path, std::ios::binary | std::ios::trunc);
        if (!f) {
            LOG_ERROR(gbemu::log::root(), "battery: failed to open {}", tmp_path);
            return;
        }
        f.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
        if (!f) {
            LOG_ERROR(gbemu::log::root(), "battery: failed to write {}", tmp_path);
            return;
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp_path, sav_path, ec);
    if (ec) {
        LOG_ERROR(gbemu::log::root(), "battery: rename {} → {} failed: {}", tmp_path, sav_path, ec.message());
        return;
    }
    cart->ram_clear_dirty();
    last_battery_flush_ms_ = SDL_GetTicks64();
}

void Application::set_rom_file(std::string_view path) {
    rom_path_ = path;
}

void Application::apply_effective_mute_(gbemu::core& core) {
    // Speed-driven mute fires for any non-1.0 user preset AND while Tab
    // is held.  ORed with the user's explicit `audio_muted` so toggling
    // Mute mid-fast-forward sticks once FF releases.  v1 keeps it simple:
    // resampling at off-rates is a separate effort (mentioned in §3).
    const bool off_rate = fast_forward_active_ || (user_state_.speed_multiplier != 1.0f);
    core.apu.set_muted(user_state_.audio_muted || off_rate);

    // At off-rate playback we must ALSO skip the ring push, not just
    // zero the samples — otherwise the APU keeps feeding the ring at
    // CPU_HZ * speed samples per second while the SDL audio callback
    // drains at exactly SAMPLE_RATE per second.  At speed > 1.0 the
    // ring fills, audio_ring_buffer::push starts yielding inside
    // emit_sample, and the audio thread becomes the wall-clock pacer:
    // the emulator caps at 1.0x and the symptom is "more choppy" with
    // no actual speedup.  enable_output(false) skips the push entirely
    // (existing semantics — also how headless mode keeps the ring from
    // deadlocking); audio underruns into silence via the callback's
    // pop-returning-false path until 1.0x is restored.
    core.apu.enable_output(!off_rate);
}

void Application::apply_speed_state_(gbemu::core& core) {
    apply_effective_mute_(core);
    last_applied_speed_ = user_state_.speed_multiplier;

    // Toggle renderer vsync on the FF edge.  SDL_RenderSetVSync returns
    // <0 on drivers that don't support runtime vsync flips (older
    // Direct3D9 builds, software renderer); we still update the cached
    // flag so we don't keep hammering the call.
    if (renderer_) {
        const bool want_disabled = fast_forward_active_;
        if (want_disabled != vsync_disabled_) {
            SDL_RenderSetVSync(renderer_, want_disabled ? 0 : 1);
            vsync_disabled_ = want_disabled;
        }
    }

    // Titlebar indicator. Plain "GbEmu" at 1.0x (no FF), otherwise append
    // the effective multiplier so the user can see the current state at
    // a glance — useful when the menu is closed.
    if (window_) {
        if (fast_forward_active_) {
            SDL_SetWindowTitle(window_, "GbEmu - FF");
        } else if (user_state_.speed_multiplier != 1.0f) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "GbEmu - %.2gx", user_state_.speed_multiplier);
            SDL_SetWindowTitle(window_, buf);
        } else {
            SDL_SetWindowTitle(window_, "GbEmu");
        }
    }
}

void Application::bump_speed_up_(gbemu::core& core) {
    const float cur = user_state_.speed_multiplier;
    // Strictly-greater so repeated presses always move forward even if
    // the current value sits exactly on a preset.  Clamps at the top end
    // (no roll-around — accidentally jumping from 4x to 0.25x would be
    // jarring and easy to do with a held key).
    for (int i = 0; i < SPEED_PRESET_COUNT; ++i) {
        if (SPEED_PRESETS[i] > cur + 1e-4f) {
            user_state_.speed_multiplier = SPEED_PRESETS[i];
            apply_speed_state_(core);
            return;
        }
    }
}

void Application::bump_speed_down_(gbemu::core& core) {
    const float cur = user_state_.speed_multiplier;
    for (int i = SPEED_PRESET_COUNT - 1; i >= 0; --i) {
        if (SPEED_PRESETS[i] < cur - 1e-4f) {
            user_state_.speed_multiplier = SPEED_PRESETS[i];
            apply_speed_state_(core);
            return;
        }
    }
}

void Application::reset_speed_(gbemu::core& core) {
    user_state_.speed_multiplier = 1.0f;
    apply_speed_state_(core);
}

void Application::handle_action_(gbemu::input::hotkey_event ev, gbemu::core& core, gbemu::debugger& dbg,
                                 gbemu::ui::context* ui_ctx) {
    using a = gbemu::input::action;
    // `pressed` distinguishes the hold-binding press edge from the
    // release edge.  Oneshot bindings always arrive with pressed=true;
    // only fast_forward currently observes the release.
    const bool pressed = ev.hold_pressed;
    switch (ev.act) {
        case a::load_rom:
            gbemu::ui::actions(ui_ctx).load_rom_dialog_requested = true;
            break;
        case a::toggle_pause:
            // ROM gate enforced by the binding's gate::rom_only — by the
            // time we land here, a cart is attached.
            dbg.toggle_running();
            break;
        case a::reset:
            dbg.reset();
            gbemu::ui::reset_display_post(ui_ctx);
            break;
        case a::toggle_fullscreen: {
            const bool is_fs = (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
            SDL_SetWindowFullscreen(window_, is_fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
            break;
        }
        case a::speed_up:
            bump_speed_up_(core);
            gbemu::ui::actions(ui_ctx).save_user_state_requested = true;
            break;
        case a::speed_down:
            bump_speed_down_(core);
            gbemu::ui::actions(ui_ctx).save_user_state_requested = true;
            break;
        case a::speed_reset:
            reset_speed_(core);
            gbemu::ui::actions(ui_ctx).save_user_state_requested = true;
            break;
        case a::fast_forward:
            // Hold semantics: pressed→engage, released→drop back to the
            // persisted preset.  apply_speed_state_ updates the title
            // and flips renderer vsync on the FF edge.
            if (pressed && !fast_forward_active_) {
                fast_forward_active_ = true;
                apply_speed_state_(core);
            } else if (!pressed && fast_forward_active_) {
                fast_forward_active_ = false;
                apply_speed_state_(core);
            }
            break;
        case a::toggle_mute:
            user_state_.audio_muted = !user_state_.audio_muted;
            apply_effective_mute_(core);
            gbemu::ui::actions(ui_ctx).save_user_state_requested = true;
            break;
        case a::volume_up:
        case a::volume_down: {
            // Snap to the next 10% bucket strictly above (or below) the
            // current value, clamped to [0,100].  Repeated presses
            // converge on round numbers regardless of where the slider
            // started — 47% + UP → 50%, not 57%.
            const int cur = static_cast<int>(user_state_.audio_volume * 100.0f + 0.5f);
            int pct = (ev.act == a::volume_up) ? ((cur / 10) + 1) * 10 : ((cur > 0) ? ((cur - 1) / 10) * 10 : 0);
            pct = std::clamp(pct, 0, 100);
            user_state_.audio_volume = static_cast<float>(pct) / 100.0f;
            core.apu.set_master_gain(user_state_.audio_volume * user_state_.audio_volume);
            gbemu::ui::actions(ui_ctx).save_user_state_requested = true;
            break;
        }
    }
}

void Application::run() {
    gbemu::core core;
    gbemu::debugger debugger{core};

    // The debugger's own $FF02 handler (installed in its ctor) is now the
    // single canonical serial sink — read by the headless `serial-dump`
    // command and rendered live by the ImGui Serial panel.  The previous
    // stdout-echo handler was retired in PR5.

    // load bios — resolved against <base>/<paths.bios_dir> when the
    // config value is a bare filename, used as-is otherwise.
    if (!cfg.bios.path.empty() && !cfg.bios.skip) {
        auto resolved = gbemu::paths::resolve_data_path(base_, cfg.paths.bios_dir, cfg.bios.path);
        LOG_INFO(gbemu::log::root(), "bios:     {}", resolved);
        load_bios(core, resolved);
    }

    // load rom — anchored under <base>/<paths.roms_dir> for bare names.
    // An empty rom_path_ is legal: the MMU treats a missing cartridge as
    // open-bus, and we pause the emulator below so the user can plug a
    // ROM in via the future File -> Load ROM menu without the CPU
    // hammering on 0xFF (RST 38h) bytes in the meantime.
    const bool rom_present = !rom_path_.empty();
    if (rom_present) {
        auto resolved = gbemu::paths::resolve_data_path(base_, cfg.paths.roms_dir, rom_path_);
        LOG_INFO(gbemu::log::root(), "rom:      {}", resolved);
        load_rom_(core, resolved);
    }

    core.init();

    if (!headless_ && !rom_present) {
        debugger.pause();
    }

    if (headless_) {
        // No window, no renderer, no event loop — just run the script and
        // return.  PPU keeps running internally; its framebuffer is just
        // never presented, so VBlank edges remain observable by run-until.
        // APU output stays disabled in headless mode: nobody is draining the
        // ring buffer, and enabling push would deadlock at the first full
        // buffer.  Sample-pacing counters still advance, so cycle accounting
        // is unaffected.
        gbemu::run_script(debugger, script_path_, output_path_);
        // Headless scripts can mutate SRAM (write to $A000-$BFFF via the
        // CPU); make sure those edits land on disk before returning so a
        // subsequent script run picks them up.
        flush_battery_save_(core);
        return;
    }

    // Open the audio device once the core (and therefore the APU's ring
    // buffer) exists.  Format is 48 kHz / F32 / stereo to match
    // gbemu::apu::SAMPLE_RATE.  `samples=1024` is the per-callback frame count
    // — small enough to keep latency low (~21 ms at 48 kHz) without thrashing
    // the audio thread.  We use SDL_AUDIO_ALLOW_FREQUENCY_CHANGE=0 (default)
    // so SDL is forced to give us exactly the rate we asked for; otherwise
    // the APU's sample cadence would drift from the actual device rate.
    SDL_AudioDeviceID audio_dev = 0;
    {
        SDL_AudioSpec want{}, have{};
        want.freq = gbemu::apu::SAMPLE_RATE;
        want.format = AUDIO_F32SYS;
        want.channels = 2;
        want.samples = 1024;
        want.callback = audio_callback;
        want.userdata = &core.apu.output();
        audio_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (audio_dev == 0)
            throw gbemu::gbemu_exception{"SDL_OpenAudioDevice failed!"};
        core.apu.enable_output(true);
        SDL_PauseAudioDevice(audio_dev, 0); // start the audio thread
    }

    // Load persisted session state.  user.conf lives under
    // <base>/<user_dir>/ alongside imgui.ini so all interactive-mode
    // state stays self-contained in the portable layout (no more CWD-
    // relative gbemu_window.state / gbemu_recent.txt).  Missing file on
    // first run is normal: load() leaves user_state_ at its defaults.
    // Ensure the directory exists up front so ImGui's first
    // SaveIniSettingsToDisk doesn't silently no-op on a fresh install
    // (CMake materialises user/ at configure time in the dev tree, but a
    // packaged release running from a fresh layout might not have it).
    user_conf_path_ = gbemu::paths::resolve_data_path(base_, cfg.paths.user_dir, "user.conf");
    const auto imgui_ini_path = gbemu::paths::resolve_data_path(base_, cfg.paths.user_dir, "imgui.ini");
    {
        std::error_code ec;
        std::filesystem::create_directories(gbemu::paths::resolve_under(base_, cfg.paths.user_dir), ec);
    }
    user_state_.load(user_conf_path_);

    // Push persisted audio prefs into the APU now that user_state has been
    // loaded and the ROM is attached (so cgb_mode is correct for the
    // accurate-filter cutoff).  Volume slider is stored as 0..1 linear in
    // user_state; the APU receives the squared value so the perceived
    // taper is closer to log.  Unknown highpass strings (forward-compat /
    // typo) fall back to "accurate" matching the load-time default.
    {
        core.apu.set_muted(user_state_.audio_muted);
        core.apu.set_master_gain(user_state_.audio_volume * user_state_.audio_volume);
        using hp = gbemu::apu::highpass_mode;
        hp mode = hp::accurate;
        if (user_state_.audio_highpass == "off")
            mode = hp::off;
        else if (user_state_.audio_highpass == "preserve")
            mode = hp::preserve;
        else if (user_state_.audio_highpass != "accurate")
            user_state_.audio_highpass = "accurate";
        core.apu.set_highpass_mode(mode);
    }

    // Restore last-used SDL window dimensions if available; otherwise fall
    // back to the hard-coded default (first launch on this machine).  A
    // zero in either dimension is treated as "no saved value" — covers
    // both a fresh user.conf and a partial/corrupt write.
    int win_w = (user_state_.window_w > 0) ? user_state_.window_w : DEFAULT_WINDOW_WIDTH;
    int win_h = (user_state_.window_h > 0) ? user_state_.window_h : DEFAULT_WINDOW_HEIGHT;

    auto window = SDL_CreateWindow("GbEmu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window)
        throw gbemu::gbemu_exception{"SDL Window creation failed!"};

    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
        throw gbemu::gbemu_exception{"SDL Renderer creation failed!"};

    // Stash SDL handles so the speed helpers (which may be invoked from
    // hotkeys, the UI menu drain pass, or apply_speed_state_ on init)
    // can update the window title and renderer vsync without threading
    // arguments through every site.
    window_ = window;
    renderer_ = renderer;

    // Clamp persisted speed onto the preset window before the first
    // frame burns a budget derived from a bogus value.  An out-of-range
    // user.conf (hand-edited, or from a future schema with extra
    // presets) snaps to the nearest preset; sub-preset values round to
    // 1.0x rather than silently picking 0.25x.
    {
        float& s = user_state_.speed_multiplier;
        const float lo = SPEED_PRESETS[0];
        const float hi = SPEED_PRESETS[SPEED_PRESET_COUNT - 1];
        if (!(s == s) || s < lo - 1e-4f || s > hi + 1e-4f)
            s = 1.0f;
    }
    apply_speed_state_(core);

    // gfx::backend wraps the SDL_Renderer so the UI can spawn presenters
    // for the GB display and the PPU panel's tile/map viewers without
    // depending on SDL_Texture directly.  When/if an OpenGL3 backend lands,
    // only this factory call and the imgui backend init in ui::init swap;
    // ui.cpp stays untouched.
    auto* gfx_backend = gbemu::gfx::sdl_backend_create(renderer);
    if (!gfx_backend)
        throw gbemu::gbemu_exception{"gfx backend creation failed!"};

    auto* ui_ctx = gbemu::ui::init(window, renderer, gfx_backend, debugger, core, user_state_, input_, imgui_ini_path);

    // Apply display config (frame blending mode + active palette) and
    // scan the palettes directory for any user .sbp files.  Has to run
    // after ui::init (which constructs the post-processor) and before
    // the main loop pumps frames so the first VBlank renders with the
    // right palette.
    {
        const auto palettes_dir = gbemu::paths::resolve_under(base_, cfg.paths.palettes_dir);
        gbemu::ui::apply_display_config(ui_ctx, cfg, palettes_dir);
    }

    // Open the first available game controller, if any. The matching
    // SDL_CONTROLLERDEVICEADDED event also fires in the main loop, so
    // hotplug works the same way — this just covers the case where a
    // controller was already plugged in at startup.
    SDL_GameController* controller = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller)
                break;
        }
    }

    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            const bool imgui_captured = gbemu::ui::process_event(ui_ctx, e);

            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            } else if (e.type == SDL_KEYDOWN) {
                // Hotkey dispatch.  Application-level shortcuts (Space,
                // Ctrl+R, F11, …) come back from input_ as a semantic
                // `action` and route through handle_action_; the joypad
                // pass-through is a second, independent sweep on the
                // same key event.  Both sides are gated on
                // !imgui_captured today to preserve historical behavior
                // (the §17 spec calls out the joypad-no-gate variant as
                // a future tweak — see input::manager docstring).
                const bool rom_loaded = core.mmu.cart() != nullptr;
                if (auto ev = input_.on_key_down(e.key.keysym.sym, e.key.keysym.mod, e.key.repeat, imgui_captured,
                                                 rom_loaded)) {
                    handle_action_(*ev, core, debugger, ui_ctx);
                }
                if (!imgui_captured && !e.key.repeat) {
                    gbemu::joypad::button btn;
                    if (input_.on_key_for_joypad(e.key.keysym.sym, btn))
                        core.joypad.set_button(btn, true);
                }
            } else if (e.type == SDL_KEYUP) {
                // Hold-kind hotkey release (Tab → fast-forward off).
                // imgui_captured does NOT gate the release: a held key
                // whose release lands inside an ImGui text field must
                // still produce its release event or fast-forward
                // sticks indefinitely.
                if (auto ev = input_.on_key_up(e.key.keysym.sym))
                    handle_action_(*ev, core, debugger, ui_ctx);
                if (!imgui_captured) {
                    gbemu::joypad::button btn;
                    if (input_.on_key_for_joypad(e.key.keysym.sym, btn))
                        core.joypad.set_button(btn, false);
                }
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                gbemu::joypad::button btn;
                if (input_.on_controller_button(e.cbutton.button, btn))
                    core.joypad.set_button(btn, true);
            } else if (e.type == SDL_CONTROLLERBUTTONUP) {
                gbemu::joypad::button btn;
                if (input_.on_controller_button(e.cbutton.button, btn))
                    core.joypad.set_button(btn, false);
            } else if (e.type == SDL_CONTROLLERDEVICEADDED) {
                // Hot-plug: claim the new pad only if we don't already
                // have one open. e.cdevice.which is a *joystick index* on
                // CONTROLLERDEVICEADDED (per SDL docs), suitable for
                // SDL_GameControllerOpen.
                if (!controller)
                    controller = SDL_GameControllerOpen(e.cdevice.which);
            } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
                // Here e.cdevice.which is an *instance id*. Match against
                // the open controller's instance id and close if it's the
                // one that went away.
                if (controller) {
                    SDL_Joystick* j = SDL_GameControllerGetJoystick(controller);
                    if (j && SDL_JoystickInstanceID(j) == e.cdevice.which) {
                        SDL_GameControllerClose(controller);
                        controller = nullptr;
                    }
                }
            }
        }

        // Step the emulator only when the CPU panel hasn't paused it.  ImGui
        // keeps drawing either way so the user can inspect state and use the
        // Step / Step Over buttons.  In Run mode we route through
        // `run_until(none)` so that breakpoints and watchpoints set via the UI
        // actually fire — they auto-pause the debugger when they hit.
        if (!debugger.is_paused()) {
            gbemu::stop_condition cond{};
            cond.kind = gbemu::stop_kind::none;
            // run_until budget is in CPU-clock T-cycles (same domain as
            // total_cycles).  In CGB double-speed the CPU emits twice as
            // many T-cycles per wall-clock frame, so the per-frame budget
            // doubles to keep the PPU stepping at the same 60 Hz rate.
            const std::uint64_t base_budget = gb::CYCLES_PER_FRAME * (core.cpu.double_speed ? 2 : 1);

            auto run_one_chunk = [&](std::uint64_t b) -> bool {
                const auto rr = debugger.run_until(cond, b);
                if (rr.outcome == gbemu::run_outcome::breakpoint || rr.outcome == gbemu::run_outcome::watchpoint) {
                    debugger.pause();
                    return false;
                }
                return true;
            };

            if (fast_forward_active_) {
                // Wall-clock-budgeted multi-frame burst with vsync off.
                // Each iteration runs one nominal frame, so the achieved
                // speedup tops out at host throughput.  The 10 ms ceiling
                // leaves room for the actual render + present that
                // follows below; the iteration cap is a belt-and-braces
                // against pathological hosts where the deadline check is
                // slow.
                const std::uint64_t deadline = SDL_GetTicks64() + 10;
                for (int i = 0; i < 64; ++i) {
                    if (!run_one_chunk(base_budget))
                        break;
                    if (SDL_GetTicks64() >= deadline)
                        break;
                }
            } else {
                const float spd = user_state_.speed_multiplier;
                std::uint64_t budget = base_budget;
                if (spd > 1.0f) {
                    // Scale up the budget: more emulation per wall-clock
                    // frame, vsync still pacing real time.
                    budget =
                        static_cast<std::uint64_t>(static_cast<double>(base_budget) * static_cast<double>(spd) + 0.5);
                }
                run_one_chunk(budget);
                if (spd < 1.0f && spd > 0.0f) {
                    // Slow down by adding wall-clock delay on top of the
                    // vsynced ~16.67 ms frame so the effective frame
                    // length matches (1/spd) * 16.67 ms.  SDL_Delay's
                    // millisecond granularity is good enough for the UX
                    // bucket we expose (slowest is 0.25x = ~50 ms extra).
                    const float extra_ms = (1.0f / spd - 1.0f) * 16.667f;
                    if (extra_ms > 0.5f)
                        SDL_Delay(static_cast<std::uint32_t>(extra_ms));
                }
            }
        }

        // The PPU's frame-ready edge is now consumed inside ui::render_frame
        // (the UI owns the display presenter and refreshes it before drawing).
        SDL_RenderClear(renderer);
        gbemu::ui::render_frame(ui_ctx);
        SDL_RenderPresent(renderer);

        // Drain UI -> Application requests after the frame is on screen so
        // blocking ops (native file dialog) and state-mutating ones (ROM
        // hot-swap) never run inside NewFrame/Render.  See ui::host_actions
        // for the contract.
        auto& acts = gbemu::ui::actions(ui_ctx);
        if (acts.quit_requested) {
            acts.quit_requested = false;
            quit = true;
        }
        if (acts.load_rom_dialog_requested) {
            acts.load_rom_dialog_requested = false;
            const auto roms_dir = gbemu::paths::resolve_under(base_, cfg.paths.roms_dir);
            auto picked = open_rom_dialog(window, roms_dir);
            if (!picked.empty())
                acts.pending_rom_load = std::move(picked);
        }
        if (!acts.pending_rom_load.empty()) {
            auto path = std::move(acts.pending_rom_load);
            acts.pending_rom_load.clear();
            try {
                // Flush the *outgoing* cart's battery save before swapping
                // it out — the previous current_rom_hash_ still points at
                // the file we want to update.  No-op when the previous
                // cart had no battery (or no cart was loaded).
                flush_battery_save_(core);
                load_rom_(core, path);
                // reset() wipes RAM / VRAM / regs and re-runs init() with the
                // freshly attached cart in place — same path as Ctrl+R after
                // the swap, so banking state, MBC, BIOS overlay, total cycles
                // all land at power-on.  Battery RAM survives because the
                // MBC's ram_ vector is moved into the new cart untouched and
                // reset() does not zero it (load_rom_ ran ram_load() before
                // this point).
                core.reset();
                gbemu::ui::reset_display_post(ui_ctx);
                debugger.resume();
                gbemu::ui::add_recent_rom(ui_ctx, path);
                LOG_INFO(gbemu::log::root(), "Loaded ROM: {}", path);
            } catch (const std::exception& e) {
                LOG_ERROR(gbemu::log::root(), "Load ROM failed ({}): {}", path, e.what());
            }
        }
        // Persist user.conf if the UI mutated user_state during this
        // frame (recents add/clear today, palette / panel-state in the
        // future).  Drained last so a successful ROM load above also
        // catches its add_recent_rom in this same frame instead of
        // waiting for the next.
        if (acts.save_user_state_requested) {
            acts.save_user_state_requested = false;
            user_state_.save(user_conf_path_);
            // The Audio -> Mute menu writes apu.set_muted directly with
            // the raw user_state_.audio_muted value; re-apply the
            // effective mute here so the speed-mute OR gate isn't
            // bypassed when the user toggles mute mid-fast-forward.
            apply_effective_mute_(core);
        }

        // Pick up external speed mutations (UI Speed submenu writes
        // directly to user_state_).  Hotkeys already call
        // apply_speed_state_ inline, so this is a one-frame-lag fallback
        // for the menu path — and the early-out keeps it free in the
        // common case.  Comparison is exact (preset list values are
        // representable in float without drift).
        if (user_state_.speed_multiplier != last_applied_speed_) {
            apply_speed_state_(core);
        }

        // Periodic battery save flush.  Every ~2 s we check whether the
        // cart's SRAM has been written to and, if so, persist it.  RTC
        // freshness is handled implicitly: rtc_blob() always reads the
        // current decomposition (post catch_up), so each flush writes
        // the most recent saved_unix anchor.  Worst-case data loss on a
        // crash is ~2 s of unwritten SRAM + drifted RTC anchor — both
        // recovered on next boot via rtc_load_blob's wall-clock gap
        // logic.  Gated on cart presence to avoid syscall churn before
        // a ROM is loaded.
        {
            const std::uint64_t now_ms = SDL_GetTicks64();
            if (now_ms >= last_battery_flush_ms_ + 2000) {
                const auto* cart = core.mmu.cart();
                if (cart && cart->has_battery() && cart->ram_dirty())
                    flush_battery_save_(core);
                else if (cart && cart->has_battery())
                    last_battery_flush_ms_ = now_ms; // throttle no-op probes
            }
        }
    }

    // Final battery flush before tearing the core down.  We always write
    // (not just on dirty) so the RTC's saved_unix anchor moves forward to
    // the moment of shutdown — that's what rtc_load_blob keys off when
    // the user next boots.  No-op when no cart was loaded.
    flush_battery_save_(core);

    // Snapshot final window dimensions and persist the whole user_state
    // blob before teardown so the next launch re-opens at the same size
    // (and with the same recents list, palette, etc).  Failure is logged
    // by user_state::save and otherwise swallowed — we'd rather start
    // fresh than block shutdown on a disk error.
    {
        int final_w = 0, final_h = 0;
        SDL_GetWindowSize(window, &final_w, &final_h);
        user_state_.window_w = final_w;
        user_state_.window_h = final_h;
        user_state_.save(user_conf_path_);
    }

    // Close the audio device before the core (and the ring buffer it owns)
    // goes out of scope, otherwise the audio thread could fire a final
    // callback into freed memory.  enable_output(false) belt-and-braces
    // prevents any in-flight emit_sample from blocking on a ring nobody is
    // draining anymore.
    if (audio_dev) {
        SDL_PauseAudioDevice(audio_dev, 1);
        core.apu.enable_output(false);
        SDL_CloseAudioDevice(audio_dev);
    }

    if (controller)
        SDL_GameControllerClose(controller);

    gbemu::ui::shutdown(ui_ctx);
    gbemu::gfx::backend_destroy(gfx_backend);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
