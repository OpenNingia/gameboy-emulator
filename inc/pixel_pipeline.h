#pragma once

#include <cstdint>

#include <gb_layout.h>
#include <mmu.h>

namespace gbemu {

    // Identifies which palette a pixel should be resolved against.
    //
    // DMG defines three: the single BG palette (BGP) shared by background and
    // window, plus two OBJ palettes (OBP0/OBP1) selected per sprite by the OAM
    // attribute bit. CGB has eight per side, addressed via BCPS/OCPS — the
    // cgb_bg0..bg7 and cgb_obj0..obj7 entries below carry that 0..7 index
    // alongside the BG/OBJ distinction the resolver needs.
    //
    // The DMG entries double as transitional fallbacks during the CGB rollout:
    // until render_bg/window/sprites stamps the proper cgb_* id into pixel_attr
    // (steps 6 and 7 of ROAD_TO_GBC.md), resolve_cgb() treats palette_id::bg as
    // cgb_bg0, obj0 as cgb_obj0, obj1 as cgb_obj1.
    enum class palette_id : std::uint8_t {
        bg = 0,
        obj0 = 1,
        obj1 = 2,

        // CGB BG palettes 0..7 (BG palette RAM indexed via BCPS/BCPD).
        cgb_bg0 = 8,
        cgb_bg1,
        cgb_bg2,
        cgb_bg3,
        cgb_bg4,
        cgb_bg5,
        cgb_bg6,
        cgb_bg7,

        // CGB OBJ palettes 0..7 (OBJ palette RAM indexed via OCPS/OCPD).
        cgb_obj0 = 16,
        cgb_obj1,
        cgb_obj2,
        cgb_obj3,
        cgb_obj4,
        cgb_obj5,
        cgb_obj6,
        cgb_obj7,
    };

    // Per-pixel attribute carried alongside the 2-bit color index through the
    // BG/window fetch pipeline. On DMG every BG/window pixel uses the same
    // default — palette_id::bg, bank 0, no priority, no flips. The struct
    // exists so the CGB fetch (which reads a second attribute byte from VRAM
    // bank 1) can vary all five fields per tile without rerouting the rest of
    // the PPU. Plumbed today; only `id` and `bank` are actually honored by the
    // fetch, the rest are stored and consumed by the sprite-priority stage
    // (and future CGB resolver) once they start carrying real values.
    struct pixel_attr {
        palette_id id;
        std::uint8_t bank;
        bool priority;
        bool x_flip;
        bool y_flip;
    };

    inline constexpr pixel_attr DMG_BG_ATTR{palette_id::bg, 0, false, false, false};

    // One BG/window pixel's worth of output from the fetch stage: the 2-bit
    // color index together with the attribute that governed the fetch. Stored
    // per scanline so render_sprites_scanline can see both (color_index for
    // transparency and BG-priority gating; attr.priority for the CGB
    // BG-over-OBJ override, currently always false on DMG).
    struct bg_pixel {
        std::uint8_t color_index; // 0..3
        pixel_attr attr;
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
                default:
                    // CGB palette ids hitting the DMG path are a programming
                    // error — fall through to BGP so the bug is obvious on
                    // screen rather than crashing the run.
                    reg = mmu_.hwr_bgp();
                    break;
            }
            const std::uint8_t shade = static_cast<std::uint8_t>((reg >> (color_index * 2)) & 0x03);
            return (*active_palette_)[shade];
        }

        // CGB resolver: read the 15-bit BGR color from BG or OBJ palette RAM,
        // expand each 5-bit channel to 8 bits (top 3 bits replicated into the
        // low 3 — the standard "5to8" expansion). Decoding id → (side, idx):
        //   palette_id::bg                       → BG palette 0  (transitional)
        //   palette_id::obj0 / obj1              → OBJ palette 0 / 1 (transitional)
        //   palette_id::cgb_bgN  (N in 0..7)    → BG palette N
        //   palette_id::cgb_objN (N in 0..7)    → OBJ palette N
        std::uint32_t resolve_cgb(palette_id id, std::uint8_t color_index) const {
            const auto raw = static_cast<std::uint8_t>(id);
            bool is_obj;
            std::uint8_t pal_idx;
            if (raw < 8) {
                is_obj = (raw != 0);
                pal_idx = (raw == static_cast<std::uint8_t>(palette_id::obj1)) ? 1 : 0;
            } else if (raw < 16) {
                is_obj = false;
                pal_idx = static_cast<std::uint8_t>(raw - static_cast<std::uint8_t>(palette_id::cgb_bg0));
            } else {
                is_obj = true;
                pal_idx = static_cast<std::uint8_t>(raw - static_cast<std::uint8_t>(palette_id::cgb_obj0));
            }
            const std::uint8_t base = static_cast<std::uint8_t>(pal_idx * 8 + color_index * 2);
            const std::uint8_t lo = is_obj ? mmu_.cgb_obj_palette_byte(base) : mmu_.cgb_bg_palette_byte(base);
            const std::uint8_t hi = is_obj ? mmu_.cgb_obj_palette_byte(static_cast<std::uint8_t>(base + 1))
                                           : mmu_.cgb_bg_palette_byte(static_cast<std::uint8_t>(base + 1));
            const std::uint16_t bgr15 = static_cast<std::uint16_t>(lo | (hi << 8));
            const std::uint8_t r5 = static_cast<std::uint8_t>(bgr15 & 0x1F);
            const std::uint8_t g5 = static_cast<std::uint8_t>((bgr15 >> 5) & 0x1F);
            const std::uint8_t b5 = static_cast<std::uint8_t>((bgr15 >> 10) & 0x1F);
            const std::uint8_t r8 = static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2));
            const std::uint8_t g8 = static_cast<std::uint8_t>((g5 << 3) | (g5 >> 2));
            const std::uint8_t b8 = static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2));
            return 0xFF000000u | (static_cast<std::uint32_t>(r8) << 16) | (static_cast<std::uint32_t>(g8) << 8) | b8;
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
