#include <cfg.h>

#include <libconfig.h++>

using namespace gbemu;
using namespace libconfig;

config::config(std::string const& file_path) {
    Config cfg;
    cfg.readFile(file_path);

    const auto& root = cfg.getRoot();
    const auto& app = root["application"];

    // BIOS block — mandatory keys, libconfig throws if missing.
    const auto& app_bios = app["bios"];
    app_bios.lookupValue("path", bios.path);
    app_bios.lookupValue("skip", bios.skip);

    // Optional `paths` block. Each entry falls back to the default baked
    // into the struct definition if the user hasn't overridden it; this
    // keeps cfg/gbemu.conf minimal and lets old configs keep working as
    // long as they still carry the bios block.
    if (app.exists("paths")) {
        const auto& app_paths = app["paths"];
        app_paths.lookupValue("bios_dir", paths.bios_dir);
        app_paths.lookupValue("roms_dir", paths.roms_dir);
        app_paths.lookupValue("savs_dir", paths.savs_dir);
        app_paths.lookupValue("sslots_dir", paths.sslots_dir);
    }
}
