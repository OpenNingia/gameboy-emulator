#include <cfg.h>

#include <libconfig.h++>

using namespace gbemu;
using namespace libconfig;

config::config(std::string const& file_path) {
    Config cfg;
    cfg.readFile(file_path);

    const auto& root = cfg.getRoot();
    const auto& app_rom = root["application"]["rom"];
    const auto& app_bios = root["application"]["bios"];

    app_rom.lookupValue("path", rom.path);
    app_bios.lookupValue("path", bios.path);
    app_bios.lookupValue("skip", bios.skip);
}
