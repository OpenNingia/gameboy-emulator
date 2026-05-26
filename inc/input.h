#pragma once
#ifndef _H_INPUT_H_
#    define _H_INPUT_H_

#    include <cstdint>
#    include <optional>
#    include <string>
#    include <string_view>
#    include <vector>

#    include <SDL2/SDL.h>
#    include <joypad.h>

namespace gbemu::input {

    // Semantic actions invoked by hotkeys.  The enum lives in input.h so
    // both the dispatcher (input.cpp) and consumers (Application's
    // handle_action, ui.cpp's menu-label lookup) can address them by name
    // without rebuilding the binding table.
    //
    // Adding a new entry: append to the enum, add a default binding in
    // config::defaults(), wire it into Application::handle_action_.
    enum class action : std::uint8_t {
        load_rom,          // File -> Load ROM (Ctrl+O)
        toggle_pause,      // Emulation -> Resume/Pause (Space), gated on ROM
        reset,             // Emulation -> Reset (Ctrl+R), gated on ROM
        toggle_fullscreen, // Emulation -> Toggle Fullscreen (F11)
        speed_up,          // (+ / KP+)
        speed_down,        // (- / KP-)
        speed_reset,       // (0 / KP0)
        fast_forward,      // Tab — HOLD: pressed on key-down, released on key-up
        toggle_mute,       // Audio -> Mute (M)
        volume_up,         // Ctrl+Up
        volume_down,       // Ctrl+Down
    };

    // Press semantics for a key_binding.
    //   oneshot: fires once on KEYDOWN with hold_pressed=true; auto-repeat
    //            events are filtered.
    //   hold:    fires on KEYDOWN with hold_pressed=true AND on the
    //            matching KEYUP with hold_pressed=false (fast-forward).
    // Hoisted out of key_binding to dodge a member-vs-type name collision
    // (`enum class kind ... } kind = ...` makes `key_binding::kind` resolve
    // to the data member as a type-name, breaking outside spellings).
    enum class binding_kind : std::uint8_t { oneshot, hold };

    // Whether the binding requires a cartridge to be attached.  rom_only
    // matches the BeginDisabled() guards in the menu bar so Space / Ctrl+R
    // don't kick a BIOS-on-open-bus into RST 38h loops.
    enum class binding_gate : std::uint8_t { always, rom_only };

    // Binding of a key chord to an action.  mod_mask is the SDL_Keymod
    // combination required for the binding to fire; the match is STRICT
    // (mod ^ mod_mask == 0 across the meaningful bits) so Ctrl+R does NOT
    // trigger on Ctrl+Shift+R.  Only the "useful" modifier bits are
    // considered — Caps/Num/Scroll lock and the system Mode key are
    // masked out before the comparison, see normalize_mod().
    struct key_binding {
        SDL_Keycode key;
        std::uint16_t mod_mask;
        action act;
        binding_kind kind = binding_kind::oneshot;
        binding_gate gate = binding_gate::always;
    };

    // Joypad binding: a keyboard key OR a game-controller button mapped
    // to one of the eight DMG buttons.  Exactly one of (key, ctrl_btn)
    // is meaningful per binding; the inactive side is SDLK_UNKNOWN / -1.
    // Two binding tables (keyboard, controller) would be cleaner but the
    // current 16-entry unified list is easier to iterate when building
    // the rebinding UI later.
    struct joypad_binding {
        SDL_Keycode key; // SDLK_UNKNOWN if controller-only
        int ctrl_btn;    // SDL_GameControllerButton, or -1 if keyboard-only
        gbemu::joypad::button gb_btn;
    };

    // Bindings configuration.  Loaded with `defaults()` at startup and
    // (post §17 step 3) merged with whatever user.conf carries.
    struct config {
        std::vector<key_binding> hotkeys;
        std::vector<joypad_binding> joypad;

        // The hard-coded defaults that mirror the original inline event
        // loop.  Keep them in sync with Application::handle_action_ —
        // unbinding an action here means it stops responding to any key.
        static config defaults();
    };

    // Event produced by manager::on_key_*: which action fired, and (for
    // hold-kind bindings) whether this is the press or the release edge.
    // Oneshot bindings always report hold_pressed=true.
    struct hotkey_event {
        action act;
        bool hold_pressed;
    };

    // Stateless-by-config dispatcher.  The manager only knows how to map
    // SDL events to actions / joypad buttons — it has no side effects on
    // the emulator.  Application owns one instance and routes results
    // through handle_action_ / core.joypad.set_button.
    class manager {
    public:
        explicit manager(config cfg);

        // SDL_KEYDOWN → optional hotkey action.  `imgui_captured` is the
        // return of ui::process_event for this event; true means a text
        // field has focus and we should not dispatch app-level hotkeys.
        // `rom_loaded` gates the rom_only bindings.  Returns nullopt
        // when no binding matches (or the event is filtered out by
        // repeat / capture / gate).
        std::optional<hotkey_event> on_key_down(SDL_Keycode k, std::uint16_t mod, bool repeat, bool imgui_captured,
                                                bool rom_loaded) const;

        // SDL_KEYUP → optional hotkey action.  Only hold-kind bindings
        // emit on key-up (with hold_pressed=false); oneshot bindings
        // ignore the release.  imgui_captured is ignored on release
        // edges so a held key whose release lands while a text field has
        // focus still gets its release event — without this a held Tab
        // could "stick" in fast-forward if the focus shifts mid-hold.
        std::optional<hotkey_event> on_key_up(SDL_Keycode k) const;

        // Keyboard → joypad button.  Returns true iff the key is mapped;
        // `out` is set to the matching GB button.  No imgui_captured
        // check — joypad pass-through historically ignored ImGui focus.
        // (Verify and reconcile in a follow-up: today's Application gates
        //  the joypad sweep on !imgui_captured, contradicting the §17 spec.)
        bool on_key_for_joypad(SDL_Keycode k, gbemu::joypad::button& out) const;

        // Game-controller button → joypad button.  SDL's normalized
        // SDL_GameControllerButton enum is what we match against; the
        // raw vendor button id never reaches this function.
        bool on_controller_button(int ctrl_btn, gbemu::joypad::button& out) const;

        // For UI labels: build "Ctrl+R" / "Space" / "F11" / "Ctrl+Up" /
        // "+ / -" / etc. from the first binding for `act`.  Empty string
        // if the action has no binding.  Display-only — does not affect
        // dispatch.
        std::string shortcut_label(action act) const;

        config const& cfg() const { return cfg_; }
        void set_cfg(config c) { cfg_ = std::move(c); }

    private:
        config cfg_;
    };

    // ---------------------------------------------------------------------
    // String round-trip helpers — used by user_state.cpp to (de)serialize
    // `input::config` into the libconfig sub-block in user.conf, and (when
    // the rebinding UI lands, §17 step 4) by the menu to render the current
    // binding for an action.  All converters are pure functions; no SDL
    // initialisation is required to call them.
    //
    // Convention: `to_string(x)` is the canonical form (lower-snake-case for
    // enums, SDL's preferred name for keys/buttons).  `*_from_string` returns
    // a sentinel (nullopt / SDLK_UNKNOWN / -1) on unknown input so callers
    // can log + skip rather than throwing.
    // ---------------------------------------------------------------------

    std::string to_string(action a);
    std::optional<action> action_from_string(std::string_view s);

    std::string to_string(gbemu::joypad::button b);
    std::optional<gbemu::joypad::button> joypad_button_from_string(std::string_view s);

    std::string to_string(binding_kind k);
    std::optional<binding_kind> binding_kind_from_string(std::string_view s);

    std::string to_string(binding_gate g);
    std::optional<binding_gate> binding_gate_from_string(std::string_view s);

    // Modifier-mask round-trip ("Ctrl+Shift" <-> KMOD_CTRL | KMOD_SHIFT).
    // Tokens are case-insensitive, joined with '+', whitespace stripped;
    // an empty string maps to 0.  Unknown tokens yield nullopt.
    std::string mod_mask_to_string(std::uint16_t mod_mask);
    std::optional<std::uint16_t> mod_mask_from_string(std::string_view s);

    // Keycode round-trip via SDL_GetKeyName / SDL_GetKeyFromName.  Returns
    // "" / SDLK_UNKNOWN on failure so the caller can detect bogus user.conf
    // entries (key = "Bogus" -> log warning, skip the binding).
    std::string keycode_to_string(SDL_Keycode k);
    SDL_Keycode keycode_from_string(std::string_view s);

    // SDL_GameControllerButton round-trip — wraps
    // SDL_GameControllerGetStringForButton / *_GetButtonFromString so
    // user_state.cpp doesn't have to depend on the controller API
    // directly.  -1 / "" on the inactive side.
    std::string controller_button_to_string(int btn);
    int controller_button_from_string(std::string_view s);

} // namespace gbemu::input

#endif // _H_INPUT_H_
