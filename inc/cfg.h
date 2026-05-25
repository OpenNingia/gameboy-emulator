#pragma once
#include <string>

namespace gbemu {
    struct config {
        explicit config(std::string const& file_path);

        // BIOS configuration. `path` may be a bare filename (resolved
        // under <base>/<paths.bios_dir>) or an absolute path. `skip` makes
        // the application launch without copying a BIOS image into the
        // MMU, so the core jumps straight to the post-BIOS register state.
        struct bioscfg {
            std::string path;
            bool skip;
        } bios;

        // Sub-directory layout under the base directory. Each entry may be
        // overridden in cfg/gbemu.conf; defaults match the standard
        // portable layout described in CLAUDE.md.
        struct pathscfg {
            std::string bios_dir{"bios"};
            std::string roms_dir{"roms"};
            std::string savs_dir{"savs"};
            std::string sslots_dir{"sslots"};
            std::string palettes_dir{"palettes"};
            std::string user_dir{"user"};
        } paths;

        // Display post-processing: frame blending (LCD response simulation)
        // and palette swap. Both apply only in interactive mode — the
        // headless script runner never consumes frames. `palette` resolves
        // against the built-in registry (`grey` / `dmg` / `mgb` / `gbl`)
        // first, then against user-loaded .sbp files under
        // <base>/<paths.palettes_dir>; unknown names fall back to `grey`.
        struct displaycfg {
            std::string frame_blending{"accurate"}; // "disabled" | "simple" | "accurate"
            std::string palette{"dmg"};             // built-in name or user .sbp stem
        } display;
    };
} // namespace gbemu
