#include <algorithm>
#include <cctype>

#include <input.h>

namespace gbemu::input {

    namespace {
        // Filter SDL_Keymod down to the bits we actually arbitrate on.
        // Caps Lock / Num Lock / Scroll Lock are sticky toggles that should
        // not affect chord matching; the system "Mode" key (KMOD_MODE) is
        // similarly out-of-band.  Left/right copies of Shift/Ctrl/Alt/GUI
        // are folded together so a binding with mod_mask=KMOD_CTRL matches
        // both LCTRL and RCTRL.
        constexpr std::uint16_t MOD_MASK_RELEVANT =
            KMOD_LSHIFT | KMOD_RSHIFT | KMOD_LCTRL | KMOD_RCTRL | KMOD_LALT | KMOD_RALT | KMOD_LGUI | KMOD_RGUI;

        std::uint16_t normalize_mod(std::uint16_t mod) {
            const std::uint16_t m = mod & MOD_MASK_RELEVANT;
            std::uint16_t out = 0;
            if (m & (KMOD_LSHIFT | KMOD_RSHIFT))
                out |= KMOD_SHIFT;
            if (m & (KMOD_LCTRL | KMOD_RCTRL))
                out |= KMOD_CTRL;
            if (m & (KMOD_LALT | KMOD_RALT))
                out |= KMOD_ALT;
            if (m & (KMOD_LGUI | KMOD_RGUI))
                out |= KMOD_GUI;
            return out;
        }

        // Pretty-print a key code for a menu shortcut label.  Falls back
        // to SDL_GetKeyName for anything we don't special-case; that
        // covers letters/digits/F-keys with the OS's preferred display
        // name (e.g. "Space" stays "Space" on Windows).
        std::string key_label(SDL_Keycode k) {
            switch (k) {
                case SDLK_UP:
                    return "Up";
                case SDLK_DOWN:
                    return "Down";
                case SDLK_LEFT:
                    return "Left";
                case SDLK_RIGHT:
                    return "Right";
                case SDLK_EQUALS:
                    return "+";
                case SDLK_KP_PLUS:
                    return "+";
                case SDLK_MINUS:
                    return "-";
                case SDLK_KP_MINUS:
                    return "-";
                case SDLK_KP_0:
                    return "0";
                default:
                    return SDL_GetKeyName(k);
            }
        }

        std::string mod_prefix(std::uint16_t mod_mask) {
            // Standard Windows / cross-platform display order: Ctrl, Alt,
            // Shift, Gui.  We don't currently bind Shift/Alt/Gui chords
            // but the formatter handles them so the rebinding UI can pick
            // them up without revisiting this code.
            std::string s;
            if (mod_mask & KMOD_CTRL)
                s += "Ctrl+";
            if (mod_mask & KMOD_ALT)
                s += "Alt+";
            if (mod_mask & KMOD_SHIFT)
                s += "Shift+";
            if (mod_mask & KMOD_GUI)
                s += "Gui+";
            return s;
        }
    } // namespace

    config config::defaults() {
        config c;
        // Hotkeys — order matches the menu bar reading order so the
        // rebinding UI surfaces them top-down without extra sorting.
        c.hotkeys = {
            // File
            {SDLK_o, KMOD_CTRL, action::load_rom},
            // Emulation
            {SDLK_SPACE, 0, action::toggle_pause, binding_kind::oneshot, binding_gate::rom_only},
            {SDLK_r, KMOD_CTRL, action::reset, binding_kind::oneshot, binding_gate::rom_only},
            {SDLK_F11, 0, action::toggle_fullscreen},
            // Speed
            {SDLK_EQUALS, 0, action::speed_up},
            {SDLK_KP_PLUS, 0, action::speed_up},
            {SDLK_MINUS, 0, action::speed_down},
            {SDLK_KP_MINUS, 0, action::speed_down},
            {SDLK_0, 0, action::speed_reset},
            {SDLK_KP_0, 0, action::speed_reset},
            {SDLK_TAB, 0, action::fast_forward, binding_kind::hold},
            // Audio
            {SDLK_m, 0, action::toggle_mute},
            {SDLK_UP, KMOD_CTRL, action::volume_up},
            {SDLK_DOWN, KMOD_CTRL, action::volume_down},
        };

        // Joypad — keyboard half (arrows + Z/X + Backspace/Enter, the
        // de-facto fceux/SameBoy convention) and controller half (SDL's
        // normalized buttons, GB-A→south, GB-B→west, Back/Start for
        // Select/Start).  Same mapping the previous map_keycode_to_button
        // / map_controller_button_to_button switches produced.
        using b = gbemu::joypad::button;
        c.joypad = {
            // keyboard
            {SDLK_UP, -1, b::up},
            {SDLK_DOWN, -1, b::down},
            {SDLK_LEFT, -1, b::left},
            {SDLK_RIGHT, -1, b::right},
            {SDLK_z, -1, b::a},
            {SDLK_x, -1, b::b},
            {SDLK_BACKSPACE, -1, b::select},
            {SDLK_RETURN, -1, b::start},
            // controller
            {SDLK_UNKNOWN, SDL_CONTROLLER_BUTTON_DPAD_UP, b::up},
            {SDLK_UNKNOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN, b::down},
            {SDLK_UNKNOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT, b::left},
            {SDLK_UNKNOWN, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, b::right},
            {SDLK_UNKNOWN, SDL_CONTROLLER_BUTTON_A, b::a},
            {SDLK_UNKNOWN, SDL_CONTROLLER_BUTTON_X, b::b},
            {SDLK_UNKNOWN, SDL_CONTROLLER_BUTTON_BACK, b::select},
            {SDLK_UNKNOWN, SDL_CONTROLLER_BUTTON_START, b::start},
        };
        return c;
    }

    manager::manager(config cfg) : cfg_(std::move(cfg)) {}

    std::optional<hotkey_event> manager::on_key_down(SDL_Keycode k, std::uint16_t mod, bool repeat, bool imgui_captured,
                                                     bool rom_loaded) const {
        if (imgui_captured)
            return std::nullopt;
        const std::uint16_t nm = normalize_mod(mod);
        for (const auto& kb : cfg_.hotkeys) {
            if (kb.key != k)
                continue;
            if (nm != kb.mod_mask)
                continue;
            if (kb.gate == binding_gate::rom_only && !rom_loaded)
                continue;
            if (kb.kind == binding_kind::oneshot) {
                if (repeat)
                    return std::nullopt; // one-shot ignores key auto-repeat
                return hotkey_event{kb.act, true};
            }
            // hold: only the first KEYDOWN matters; auto-repeat is the
            // OS re-asserting the press, not a re-engagement.
            if (repeat)
                return std::nullopt;
            return hotkey_event{kb.act, true};
        }
        return std::nullopt;
    }

    std::optional<hotkey_event> manager::on_key_up(SDL_Keycode k) const {
        for (const auto& kb : cfg_.hotkeys) {
            if (kb.kind != binding_kind::hold)
                continue;
            if (kb.key != k)
                continue;
            // mod_mask ignored on release — the user may have let go of
            // the modifier before the main key.  Holding Tab without
            // any modifier is the only hold binding today; if Ctrl+Tab
            // becomes a hold later, revisit this.
            return hotkey_event{kb.act, false};
        }
        return std::nullopt;
    }

    bool manager::on_key_for_joypad(SDL_Keycode k, gbemu::joypad::button& out) const {
        for (const auto& jb : cfg_.joypad) {
            if (jb.key != SDLK_UNKNOWN && jb.key == k) {
                out = jb.gb_btn;
                return true;
            }
        }
        return false;
    }

    bool manager::on_controller_button(int ctrl_btn, gbemu::joypad::button& out) const {
        for (const auto& jb : cfg_.joypad) {
            if (jb.ctrl_btn >= 0 && jb.ctrl_btn == ctrl_btn) {
                out = jb.gb_btn;
                return true;
            }
        }
        return false;
    }

    std::string manager::shortcut_label(action act) const {
        for (const auto& kb : cfg_.hotkeys) {
            if (kb.act != act)
                continue;
            return mod_prefix(kb.mod_mask) + key_label(kb.key);
        }
        return {};
    }

    // -----------------------------------------------------------------
    // Serialization helpers
    // -----------------------------------------------------------------

    namespace {
        // Pair-of-(enum,string) tables keep the forward and reverse
        // converters perfectly in sync — adding a new action means one
        // edit instead of two.
        struct action_pair {
            action a;
            const char* s;
        };
        constexpr action_pair ACTION_TABLE[] = {
            {action::load_rom, "load_rom"},
            {action::toggle_pause, "toggle_pause"},
            {action::reset, "reset"},
            {action::toggle_fullscreen, "toggle_fullscreen"},
            {action::speed_up, "speed_up"},
            {action::speed_down, "speed_down"},
            {action::speed_reset, "speed_reset"},
            {action::fast_forward, "fast_forward"},
            {action::toggle_mute, "toggle_mute"},
            {action::volume_up, "volume_up"},
            {action::volume_down, "volume_down"},
        };

        struct button_pair {
            gbemu::joypad::button b;
            const char* s;
        };
        constexpr button_pair BUTTON_TABLE[] = {
            {gbemu::joypad::button::right, "right"},   {gbemu::joypad::button::left, "left"},
            {gbemu::joypad::button::up, "up"},         {gbemu::joypad::button::down, "down"},
            {gbemu::joypad::button::a, "a"},           {gbemu::joypad::button::b, "b"},
            {gbemu::joypad::button::select, "select"}, {gbemu::joypad::button::start, "start"},
        };

        // Trim ASCII whitespace from both ends.
        std::string_view trim_sv(std::string_view s) {
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
                s.remove_prefix(1);
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
                s.remove_suffix(1);
            return s;
        }

        bool ieq(std::string_view a, std::string_view b) {
            if (a.size() != b.size())
                return false;
            for (std::size_t i = 0; i < a.size(); ++i) {
                if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
                    return false;
            }
            return true;
        }
    } // namespace

    std::string to_string(action a) {
        for (const auto& p : ACTION_TABLE)
            if (p.a == a)
                return p.s;
        return {};
    }

    std::optional<action> action_from_string(std::string_view s) {
        const auto t = trim_sv(s);
        for (const auto& p : ACTION_TABLE)
            if (ieq(t, p.s))
                return p.a;
        return std::nullopt;
    }

    std::string to_string(gbemu::joypad::button b) {
        for (const auto& p : BUTTON_TABLE)
            if (p.b == b)
                return p.s;
        return {};
    }

    std::optional<gbemu::joypad::button> joypad_button_from_string(std::string_view s) {
        const auto t = trim_sv(s);
        for (const auto& p : BUTTON_TABLE)
            if (ieq(t, p.s))
                return p.b;
        return std::nullopt;
    }

    std::string to_string(binding_kind k) {
        return k == binding_kind::hold ? "hold" : "oneshot";
    }

    std::optional<binding_kind> binding_kind_from_string(std::string_view s) {
        const auto t = trim_sv(s);
        if (ieq(t, "oneshot"))
            return binding_kind::oneshot;
        if (ieq(t, "hold"))
            return binding_kind::hold;
        return std::nullopt;
    }

    std::string to_string(binding_gate g) {
        return g == binding_gate::rom_only ? "rom_only" : "always";
    }

    std::optional<binding_gate> binding_gate_from_string(std::string_view s) {
        const auto t = trim_sv(s);
        if (ieq(t, "always"))
            return binding_gate::always;
        if (ieq(t, "rom_only"))
            return binding_gate::rom_only;
        return std::nullopt;
    }

    std::string mod_mask_to_string(std::uint16_t mod_mask) {
        // Same display order as the menu-bar formatter (mod_prefix), minus
        // the trailing '+'. An empty mask serialises to "" — libconfig
        // round-trips empty strings just fine.
        std::string s;
        const auto append = [&](const char* tok) {
            if (!s.empty())
                s += '+';
            s += tok;
        };
        if (mod_mask & KMOD_CTRL)
            append("Ctrl");
        if (mod_mask & KMOD_ALT)
            append("Alt");
        if (mod_mask & KMOD_SHIFT)
            append("Shift");
        if (mod_mask & KMOD_GUI)
            append("Gui");
        return s;
    }

    std::optional<std::uint16_t> mod_mask_from_string(std::string_view s) {
        std::uint16_t out = 0;
        const auto t = trim_sv(s);
        if (t.empty())
            return out;
        std::size_t i = 0;
        while (i < t.size()) {
            // Find the next '+' (token separator).
            const std::size_t plus = t.find('+', i);
            const std::size_t end = (plus == std::string_view::npos) ? t.size() : plus;
            const auto tok = trim_sv(t.substr(i, end - i));
            if (!tok.empty()) {
                if (ieq(tok, "Ctrl") || ieq(tok, "Control"))
                    out |= KMOD_CTRL;
                else if (ieq(tok, "Shift"))
                    out |= KMOD_SHIFT;
                else if (ieq(tok, "Alt"))
                    out |= KMOD_ALT;
                else if (ieq(tok, "Gui") || ieq(tok, "Meta") || ieq(tok, "Super"))
                    out |= KMOD_GUI;
                else
                    return std::nullopt;
            }
            if (plus == std::string_view::npos)
                break;
            i = plus + 1;
        }
        return out;
    }

    std::string keycode_to_string(SDL_Keycode k) {
        if (k == SDLK_UNKNOWN)
            return {};
        const char* name = SDL_GetKeyName(k);
        if (!name || !*name)
            return {};
        return name;
    }

    SDL_Keycode keycode_from_string(std::string_view s) {
        // SDL_GetKeyFromName needs a NUL-terminated string; copy into a
        // temporary to honour the contract regardless of how the view was
        // constructed.
        std::string buf{trim_sv(s)};
        if (buf.empty())
            return SDLK_UNKNOWN;
        return SDL_GetKeyFromName(buf.c_str());
    }

    std::string controller_button_to_string(int btn) {
        if (btn < 0 || btn >= SDL_CONTROLLER_BUTTON_MAX)
            return {};
        const char* name = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(btn));
        if (!name || !*name)
            return {};
        return name;
    }

    int controller_button_from_string(std::string_view s) {
        std::string buf{trim_sv(s)};
        if (buf.empty())
            return -1;
        const SDL_GameControllerButton b = SDL_GameControllerGetButtonFromString(buf.c_str());
        return (b == SDL_CONTROLLER_BUTTON_INVALID) ? -1 : static_cast<int>(b);
    }

} // namespace gbemu::input
