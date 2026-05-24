#include <iostream>
#include <string>

#include <CLI/CLI.hpp>
#include <SDL2/SDL.h> // Renames `main` → `SDL_main` so SDL2main's WinMain (linked
                      // via SDL2::SDL2main on Windows) can dispatch to us.  Required
                      // for /SUBSYSTEM:WINDOWS builds; harmless on other platforms.
#include <app.h>
#include <log.h>

int main(int argc, char* argv[]) {
    gbemu::log::init();

    CLI::App cli{"GbEmu — Game Boy emulator"};

    std::string rom_path;
    std::string script_path;
    std::string output_path{"dump.txt"};
    bool headless = false;

    cli.add_option("rom", rom_path,
                   "ROM file. Bare filename -> resolved under <base>/<paths.roms_dir>; "
                   "path with separators -> used as-is (absolute or CWD-relative).");
    cli.add_flag("--headless", headless, "Run without an SDL window; drive emulation via --script");
    cli.add_option("--script", script_path, "Debug script to execute (required with --headless)")
        ->check(CLI::ExistingFile);
    cli.add_option("--out", output_path, "Output file for script results (default: dump.txt)");

    CLI11_PARSE(cli, argc, argv);

    if (headless && script_path.empty()) {
        std::cerr << "--headless requires --script\n";
        return 2;
    }

    LOG_INFO(gbemu::log::root(), "rom arg: {}", rom_path);

    try {
        Application app;
        if (!rom_path.empty())
            app.set_rom_file(rom_path);
        if (headless) {
            app.set_headless(true);
            app.set_script_path(script_path);
            app.set_output_path(output_path);
        }
        app.run();
    } catch (const std::exception& e) {
        LOG_ERROR(gbemu::log::root(), "Error: {}", e.what());
        gbemu::log::root()->flush_log();
        return 1;
    }
    return 0;
}
