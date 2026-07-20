#include <algorithm>
#include <filesystem>
#include <system_error>

#include <log.h>
#include <user_state.h>

#include <libconfig.h++>

namespace gbemu {

    namespace {
        // ---------- input::config <-> libconfig helpers ----------

        // Parse the `input.hotkeys` array.  Returns the parsed list; each
        // malformed entry is logged and skipped without aborting the whole
        // section so a single typo doesn't wipe every binding.
        std::vector<input::key_binding> parse_hotkeys(const libconfig::Setting& arr) {
            std::vector<input::key_binding> out;
            out.reserve(static_cast<std::size_t>(arr.getLength()));
            for (int i = 0; i < arr.getLength(); ++i) {
                const auto& e = arr[i];
                std::string key_s, mod_s, act_s, kind_s = "oneshot", gate_s = "always";
                if (!e.lookupValue("key", key_s) || !e.lookupValue("action", act_s)) {
                    LOG_WARNING(log::root(), "user_state: input.hotkeys[{}] missing key/action — skipped", i);
                    continue;
                }
                e.lookupValue("mod", mod_s); // optional; "" means no modifier
                e.lookupValue("kind", kind_s);
                e.lookupValue("gate", gate_s);

                const SDL_Keycode kc = input::keycode_from_string(key_s);
                if (kc == SDLK_UNKNOWN) {
                    LOG_WARNING(log::root(), "user_state: input.hotkeys[{}] unknown key '{}' — skipped", i, key_s);
                    continue;
                }
                const auto mm = input::mod_mask_from_string(mod_s);
                if (!mm) {
                    LOG_WARNING(log::root(), "user_state: input.hotkeys[{}] bogus mod '{}' — skipped", i, mod_s);
                    continue;
                }
                const auto act = input::action_from_string(act_s);
                if (!act) {
                    LOG_WARNING(log::root(), "user_state: input.hotkeys[{}] unknown action '{}' — skipped", i, act_s);
                    continue;
                }
                const auto kind = input::binding_kind_from_string(kind_s);
                if (!kind) {
                    LOG_WARNING(log::root(), "user_state: input.hotkeys[{}] bogus kind '{}' — skipped", i, kind_s);
                    continue;
                }
                const auto gate = input::binding_gate_from_string(gate_s);
                if (!gate) {
                    LOG_WARNING(log::root(), "user_state: input.hotkeys[{}] bogus gate '{}' — skipped", i, gate_s);
                    continue;
                }
                out.push_back(input::key_binding{kc, *mm, *act, *kind, *gate});
            }
            return out;
        }

        // Parse `input.joypad_keyboard` (only `key` + `gb` fields, no
        // controller side).  Each entry produces a joypad_binding with
        // ctrl_btn=-1.
        std::vector<input::joypad_binding> parse_joypad_keyboard(const libconfig::Setting& arr) {
            std::vector<input::joypad_binding> out;
            out.reserve(static_cast<std::size_t>(arr.getLength()));
            for (int i = 0; i < arr.getLength(); ++i) {
                const auto& e = arr[i];
                std::string key_s, gb_s;
                if (!e.lookupValue("key", key_s) || !e.lookupValue("gb", gb_s)) {
                    LOG_WARNING(log::root(), "user_state: input.joypad_keyboard[{}] missing key/gb — skipped", i);
                    continue;
                }
                const SDL_Keycode kc = input::keycode_from_string(key_s);
                if (kc == SDLK_UNKNOWN) {
                    LOG_WARNING(log::root(), "user_state: input.joypad_keyboard[{}] unknown key '{}' — skipped", i,
                                key_s);
                    continue;
                }
                const auto gb = input::joypad_button_from_string(gb_s);
                if (!gb) {
                    LOG_WARNING(log::root(), "user_state: input.joypad_keyboard[{}] unknown gb '{}' — skipped", i,
                                gb_s);
                    continue;
                }
                out.push_back(input::joypad_binding{kc, -1, *gb});
            }
            return out;
        }

        // Parse `input.joypad_controller` (`btn` + `gb`).  key=SDLK_UNKNOWN.
        std::vector<input::joypad_binding> parse_joypad_controller(const libconfig::Setting& arr) {
            std::vector<input::joypad_binding> out;
            out.reserve(static_cast<std::size_t>(arr.getLength()));
            for (int i = 0; i < arr.getLength(); ++i) {
                const auto& e = arr[i];
                std::string btn_s, gb_s;
                if (!e.lookupValue("btn", btn_s) || !e.lookupValue("gb", gb_s)) {
                    LOG_WARNING(log::root(), "user_state: input.joypad_controller[{}] missing btn/gb — skipped", i);
                    continue;
                }
                const int cb = input::controller_button_from_string(btn_s);
                if (cb < 0) {
                    LOG_WARNING(log::root(), "user_state: input.joypad_controller[{}] unknown btn '{}' — skipped", i,
                                btn_s);
                    continue;
                }
                const auto gb = input::joypad_button_from_string(gb_s);
                if (!gb) {
                    LOG_WARNING(log::root(), "user_state: input.joypad_controller[{}] unknown gb '{}' — skipped", i,
                                gb_s);
                    continue;
                }
                out.push_back(input::joypad_binding{SDLK_UNKNOWN, cb, *gb});
            }
            return out;
        }

        // Write the full input::config to a freshly added `input` group.
        // Format mirrors the parse helpers above; comments in user.conf are
        // not preserved by libconfig but the schema is stable enough that
        // an external editor can be pointed at the TODO §17 docs.
        void write_input(libconfig::Setting& parent, const input::config& cfg) {
            auto& g = parent.add("input", libconfig::Setting::TypeGroup);

            auto& hot = g.add("hotkeys", libconfig::Setting::TypeList);
            for (const auto& kb : cfg.hotkeys) {
                auto& e = hot.add(libconfig::Setting::TypeGroup);
                e.add("key", libconfig::Setting::TypeString) = input::keycode_to_string(kb.key);
                e.add("mod", libconfig::Setting::TypeString) = input::mod_mask_to_string(kb.mod_mask);
                e.add("action", libconfig::Setting::TypeString) = input::to_string(kb.act);
                e.add("kind", libconfig::Setting::TypeString) = input::to_string(kb.kind);
                e.add("gate", libconfig::Setting::TypeString) = input::to_string(kb.gate);
            }

            auto& jk = g.add("joypad_keyboard", libconfig::Setting::TypeList);
            for (const auto& jb : cfg.joypad) {
                if (jb.key == SDLK_UNKNOWN)
                    continue;
                auto& e = jk.add(libconfig::Setting::TypeGroup);
                e.add("key", libconfig::Setting::TypeString) = input::keycode_to_string(jb.key);
                e.add("gb", libconfig::Setting::TypeString) = input::to_string(jb.gb_btn);
            }

            auto& jc = g.add("joypad_controller", libconfig::Setting::TypeList);
            for (const auto& jb : cfg.joypad) {
                if (jb.ctrl_btn < 0)
                    continue;
                auto& e = jc.add(libconfig::Setting::TypeGroup);
                e.add("btn", libconfig::Setting::TypeString) = input::controller_button_to_string(jb.ctrl_btn);
                e.add("gb", libconfig::Setting::TypeString) = input::to_string(jb.gb_btn);
            }
        }
    } // namespace

    bool user_state::load(const std::string& path) {
        libconfig::Config cfg;
        try {
            cfg.readFile(path.c_str());
        } catch (const std::exception&) {
            // Missing file (first run) or parse error -> keep defaults.
            // We don't distinguish the two: in either case the right
            // behaviour is "start fresh" without complaining.
            return false;
        }

        const auto& root = cfg.getRoot();

        if (root.exists("window")) {
            const auto& w = root["window"];
            w.lookupValue("width", window_w);
            w.lookupValue("height", window_h);
        }

        if (root.exists("recent_roms")) {
            const auto& list = root["recent_roms"];
            recent_roms.clear();
            recent_roms.reserve(static_cast<std::size_t>(list.getLength()));
            for (int i = 0; i < list.getLength(); ++i) {
                if (list[i].getType() == libconfig::Setting::TypeString)
                    recent_roms.emplace_back(static_cast<const char*>(list[i]));
            }
        }

        if (root.exists("display")) {
            const auto& d = root["display"];
            d.lookupValue("active_palette", active_palette);
        }

        if (root.exists("audio")) {
            const auto& a = root["audio"];
            a.lookupValue("muted", audio_muted);
            // libconfig stores numeric scalars as either int or float
            // depending on whether the literal had a dot; lookupValue<float>
            // accepts only floats, so we look up double and narrow.
            double vol_d = static_cast<double>(audio_volume);
            if (a.lookupValue("volume", vol_d))
                audio_volume = static_cast<float>(vol_d);
            a.lookupValue("highpass", audio_highpass);
        }

        if (root.exists("emulation")) {
            const auto& em = root["emulation"];
            double spd_d = static_cast<double>(speed_multiplier);
            if (em.lookupValue("speed", spd_d))
                speed_multiplier = static_cast<float>(spd_d);
        }

        if (root.exists("input")) {
            const auto& in = root["input"];
            // Each sub-array is parsed independently of the others so a
            // hand-edited user.conf that overrides only one section keeps
            // the rest at defaults.  Presence of an empty list IS a valid
            // user choice ("unbind everything in this section") — we don't
            // re-inject defaults when the parsed result is empty.
            if (in.exists("hotkeys") && in["hotkeys"].isList())
                input_bindings.hotkeys = parse_hotkeys(in["hotkeys"]);
            if (in.exists("joypad_keyboard") && in["joypad_keyboard"].isList()) {
                input_bindings.joypad.erase(
                    std::remove_if(input_bindings.joypad.begin(), input_bindings.joypad.end(),
                                   [](const input::joypad_binding& jb) { return jb.key != SDLK_UNKNOWN; }),
                    input_bindings.joypad.end());
                auto parsed = parse_joypad_keyboard(in["joypad_keyboard"]);
                input_bindings.joypad.insert(input_bindings.joypad.end(), parsed.begin(), parsed.end());
            }
            if (in.exists("joypad_controller") && in["joypad_controller"].isList()) {
                input_bindings.joypad.erase(
                    std::remove_if(input_bindings.joypad.begin(), input_bindings.joypad.end(),
                                   [](const input::joypad_binding& jb) { return jb.ctrl_btn >= 0; }),
                    input_bindings.joypad.end());
                auto parsed = parse_joypad_controller(in["joypad_controller"]);
                input_bindings.joypad.insert(input_bindings.joypad.end(), parsed.begin(), parsed.end());
            }
        }

        return true;
    }

    bool user_state::save(const std::string& path) const {
        libconfig::Config cfg;
        auto& root = cfg.getRoot();

        auto& window = root.add("window", libconfig::Setting::TypeGroup);
        window.add("width", libconfig::Setting::TypeInt) = window_w;
        window.add("height", libconfig::Setting::TypeInt) = window_h;

        // libconfig arrays may be empty; the element type is fixed at
        // first insert (TypeString here) and irrelevant when no entries
        // exist.  Round-trip through readFile -> getLength()==0 leaves
        // `recent_roms` cleared, which matches the in-memory default.
        auto& list = root.add("recent_roms", libconfig::Setting::TypeArray);
        for (const auto& s : recent_roms)
            list.add(libconfig::Setting::TypeString) = s.c_str();

        auto& display = root.add("display", libconfig::Setting::TypeGroup);
        display.add("active_palette", libconfig::Setting::TypeString) = active_palette.c_str();

        auto& audio = root.add("audio", libconfig::Setting::TypeGroup);
        audio.add("muted", libconfig::Setting::TypeBoolean) = audio_muted;
        audio.add("volume", libconfig::Setting::TypeFloat) = static_cast<double>(audio_volume);
        audio.add("highpass", libconfig::Setting::TypeString) = audio_highpass.c_str();

        auto& emulation = root.add("emulation", libconfig::Setting::TypeGroup);
        emulation.add("speed", libconfig::Setting::TypeFloat) = static_cast<double>(speed_multiplier);

        write_input(root, input_bindings);

        try {
            std::filesystem::path p{path};
            if (p.has_parent_path()) {
                std::error_code ec;
                std::filesystem::create_directories(p.parent_path(), ec);
                // create_directories failures are non-fatal: if the path
                // is truly bad the writeFile call below will throw a
                // libconfig::FileIOException which we catch and log.
            }

            const auto tmp = path + ".tmp";
            cfg.writeFile(tmp.c_str());

            std::error_code ec;
            std::filesystem::rename(tmp, path, ec);
            if (ec) {
                LOG_ERROR(log::root(), "user_state: rename {} -> {} failed: {}", tmp, path, ec.message());
                return false;
            }
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR(log::root(), "user_state: save to {} failed: {}", path, e.what());
            return false;
        }
    }

} // namespace gbemu
