#include <filesystem>
#include <system_error>

#include <log.h>
#include <user_state.h>

#include <libconfig.h++>

namespace gbemu {

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
