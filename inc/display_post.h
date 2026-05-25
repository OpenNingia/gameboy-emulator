#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <gb_layout.h>

namespace gbemu {

    // Display post-processing — LCD response simulation (frame blending) and
    // palette management. Sits between the PPU framebuffer and the gfx
    // presenter. Interactive-mode only: headless never consumes frames so
    // none of this runs in script_runner builds.
    //
    // Two independent layers:
    //   * Frame blending — keeps a small ring of recent framebuffers and
    //     mixes them with weights tuned to match the real DMG's slow LCD
    //     response. Without this, games that rely on rapid flicker for
    //     pseudo-transparency (Wario Land, Mole Mania) just strobe.
    //   * Palette swap — sets the active four-shade table on the PPU's
    //     palette_resolver. Built-in palettes mirror SameBoy's RGB values;
    //     user .sbp files dropped under <base>/<palettes_dir> are scanned
    //     at startup and added to the registry.
    namespace display {

        // Frame blending modes. Exposed verbatim in cfg/gbemu.conf — the
        // parser converts the string to this enum at boot.
        enum class blend_mode {
            disabled, // pass-through; framebuffer goes out unchanged
            simple,   // 50/50 mean of current frame and previous frame
            accurate, // 5-tap weighted decay (closest to a real DMG)
        };

        blend_mode parse_blend_mode(std::string_view name); // unknown → disabled

        // One 4-shade ARGB palette. Used as the live table on the PPU's
        // resolver and as the value type stored in the registry.
        struct palette {
            std::string name;                      // registry key (built-in id or file stem)
            std::array<std::uint32_t, 4> shades{}; // shade 0 (light) … shade 3 (dark)
            std::uint32_t off_lcd_argb{0};         // 5-color .sbp only; 0 = absent
        };

        // Process-wide registry of available palettes. Populated at boot:
        // built-ins are added first, then any *.sbp file found under the
        // palettes directory. Lookups are linear (small N), case-sensitive.
        struct palette_registry {
            // Insert built-in palettes (grey / dmg / mgb / gbl). Idempotent —
            // safe to call multiple times.
            void install_builtins();

            // Scan `dir` for *.sbp files, parse each, add valid entries.
            // Invalid files (wrong magic, bad flags, truncated, etc.) are
            // logged at warning level and skipped — never throws.
            void scan_directory(const std::string& dir);

            // Lookup by name. Returns nullptr if not found.
            const palette* find(std::string_view name) const;

            // All palettes in insertion order (built-ins first, then user
            // files in directory-iterator order). Stable references for the
            // lifetime of the registry.
            const std::vector<palette>& all() const { return entries_; }

        private:
            std::vector<palette> entries_;
        };

        // SameBoy portable .sbp file layout (per LIJI32's user-palette
        // export, see Cocoa/GBPaletteEditorController.m):
        //   bytes 0..3  : ASCII "LPBS" (magic)
        //   byte 4      : flags: 0x01 = 4-color, 0x03 = 5-color (extra =
        //                 off-LCD background, lighter than shade 0)
        //   bytes 5..   : RGB triples, darkest to lightest. 12 bytes for
        //                 4-color, 15 bytes for 5-color.
        // Returns true and fills `out` on success; returns false on any
        // validation failure (magic, flags, size, IO error). The .sbp
        // shade ordering (darkest-first) is inverted to our convention
        // (shade 0 = lightest) inside this parser.
        bool load_sbp_file(const std::string& path, palette& out);

        // Number of historical framebuffers kept by `frame_blender`. Tuned
        // for `blend_mode::accurate` which mixes the current frame with the
        // last 4 (total 5 taps); `simple` uses only the most recent slot
        // and `disabled` uses none.
        constexpr std::size_t HISTORY_SLOTS = 4;

        // Frame blender. Owns the history ring and the output buffer.
        // `blend(current)` writes the blended pixels into the output and
        // returns its pointer (or `current` verbatim when mode == disabled).
        // Not thread-safe — call from the UI thread only.
        struct frame_blender {
            frame_blender();

            void set_mode(blend_mode m);
            blend_mode mode() const { return mode_; }

            // Erase history. Called on ROM load and on core::reset so a
            // post-reset frame doesn't ghost the pre-reset image.
            void reset();

            // Blend `current` (a pointer to gb::LCD_WIDTH*gb::LCD_HEIGHT
            // ARGB pixels owned by the PPU) with the on-board history.
            // Returns a pointer to the framebuffer the caller should upload
            // to the presenter. The pointer is owned by the blender and
            // remains valid until the next call. With mode == disabled the
            // return value aliases `current` directly (zero copy).
            const std::uint32_t* blend(const std::uint32_t* current);

        private:
            static constexpr std::size_t FB_PIXELS =
                static_cast<std::size_t>(gb::LCD_WIDTH) * static_cast<std::size_t>(gb::LCD_HEIGHT);

            blend_mode mode_{blend_mode::disabled};
            // Ring of recent frames. `history_[0]` is the most recent
            // previously-uploaded frame; the writer index advances after
            // each blend in disabled / simple / accurate modes alike so
            // switching modes mid-stream stays well-defined.
            std::array<std::array<std::uint32_t, FB_PIXELS>, HISTORY_SLOTS> history_{};
            std::array<std::uint32_t, FB_PIXELS> output_{};
            std::size_t writer_{0};
            // Set once at least one frame has been pushed into history_;
            // before that the blender falls back to pass-through even in
            // simple / accurate mode (no point averaging with all-zero
            // slots, which would dim the first visible frame).
            bool history_seeded_{false};
        };

    } // namespace display
} // namespace gbemu
