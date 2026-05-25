#pragma once

#include <cstdint>

#include <gb_layout.h>
#include <mmu.h>

namespace gbemu {

    // Identifies which palette register a pixel should be resolved against.
    // DMG defines three: the single BG palette (BGP) shared by background and
    // window, plus two OBJ palettes (OBP0/OBP1) selected per sprite by the OAM
    // attribute bit. CGB extends this to bg0..bg7 / obj0..obj7 — when that
    // lands, this enum grows and palette_resolver gains a CGB code path.
    enum class palette_id : std::uint8_t {
        bg = 0,
        obj0 = 1,
        obj1 = 2,
    };

    // Maps (palette, 2-bit color index) → final ARGB8888 pixel.
    //
    // The resolver is the single choke point where DMG-specific shading lives:
    // BGP/OBP0/OBP1 mod the 4-shade table. The CGB pipeline produces the same
    // (color_index, palette_id) intermediate but routes through BCPD/OCPD
    // with 5-5-5 → 8-8-8 expansion. Keeping that decision behind this one
    // entrypoint means the BG / window / OBJ scanline code does not have to
    // change to support CGB.
    //
    // The 4-shade table itself is swappable at runtime via `set_palette` so
    // the user can pick between built-in palettes (grey / SameBoy DMG / MGB
    // / GBL) and user-loaded *.sbp files without touching the PPU. Default
    // points at `gb::DMG_PALETTE_ARGB` (classic four-shade grey) so the
    // resolver behaves exactly like before until `set_palette` is called.
    //
    // Holds only an mmu reference + a palette pointer; constructing one is
    // free.
    struct palette_resolver {
        explicit palette_resolver(const mmu& m) : mmu_(m) {}

        void set_palette(const std::array<std::uint32_t, 4>& p) { active_palette_ = &p; }

        std::uint32_t resolve(palette_id id, std::uint8_t color_index) const {
            std::uint8_t reg = 0;
            switch (id) {
                case palette_id::bg:
                    reg = mmu_.hwr_bgp();
                    break;
                case palette_id::obj0:
                    reg = mmu_.hwr_obp0();
                    break;
                case palette_id::obj1:
                    reg = mmu_.hwr_obp1();
                    break;
            }
            const std::uint8_t shade = static_cast<std::uint8_t>((reg >> (color_index * 2)) & 0x03);
            return (*active_palette_)[shade];
        }

    private:
        const mmu& mmu_;
        const std::array<std::uint32_t, 4>* active_palette_ = &gb::DMG_PALETTE_ARGB;
    };

    // Decode one 2bpp planar tile pixel: returns the 2-bit color index (0..3)
    // at column `col` (0..7) of the row formed by the (lo, hi) byte pair.
    inline std::uint8_t tile_color_index(std::uint8_t lo, std::uint8_t hi, std::uint8_t col) {
        return static_cast<std::uint8_t>((((hi >> col) & 1) << 1) | ((lo >> col) & 1));
    }

} // namespace gbemu
