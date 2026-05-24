#pragma once

#include <string>
#include <string_view>

#include <cfg.h>

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
};
