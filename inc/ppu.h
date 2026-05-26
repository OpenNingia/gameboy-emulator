#pragma once

#include <array>
#include <cstdint>

#include <gb_layout.h>
#include <mmu.h>
#include <pixel_pipeline.h>

namespace gbemu {
    struct irq;

    struct ppu {
        // PPU mode — underlying values match bits 0-1 of the STAT register,
        // so writing the mode to STAT is just `(stat & ~mode_mask) | uint8_t(mode)`.
        enum class mode_e : std::uint8_t {
            HBLANK = 0,
            VBLANK = 1,
            OAM_SCAN = 2,
            DRAWING = 3,
        };

        ppu(mmu& m, irq& i);
        void step(std::uint32_t t_cycles);
        const std::uint32_t* framebuffer() const { return fb.data(); }
        bool consume_frame_ready(); // edge-trigger SDL present
        // Park the state machine at OAM_SCAN/line 0 and blank the framebuffer.
        // Mirrors the LCD-off branch of step() but is callable from core::reset
        // before the next step has run.
        void reset();

        // H-Blank edge hook: invoked from enter_hblank() with the H-Blank
        // mode about to begin.  Wired by core::core() to drive the CGB
        // HDMA engine (which copies one 16-byte block per fire when armed).
        // Kept as a free C-style fn ptr for the same reason cpu.tick_fn is.
        using hblank_fn_t = void (*)(void* ctx);
        hblank_fn_t hblank_fn{nullptr};
        void* hblank_ctx{nullptr};

        // Active palette accessor. The PPU owns the single palette_resolver
        // shared by BG/window/sprite rendering and by the UI's tile / BG-map
        // viewers (which call resolve() to colour their own off-screen
        // pixmaps). Swapping the four-shade table here changes both at once.
        const palette_resolver& palette() const { return resolver_; }
        void set_palette(const std::array<std::uint32_t, 4>& p) { resolver_.set_palette(p); }

        // Public dispatch wrapper around the DMG vs CGB resolver paths.
        // The OAM viewer in ui.cpp uses this so the sprite preview honours
        // CGB OBJ palettes (bits 0-2 of the attribute byte) without having
        // to duplicate the cgb_mode() branch.  Used internally by the BG /
        // window / sprite render passes as well — exposed here so the same
        // entry point is reused rather than re-derived on the caller side.
        std::uint32_t resolve_pixel(palette_id id, std::uint8_t color_index) const {
            return mmu_.cgb_mode() ? resolver_.resolve_cgb(id, color_index) : resolver_.resolve(id, color_index);
        }

    private:
        mmu& mmu_;
        irq& irq_;
        palette_resolver resolver_;
        mode_e mode{mode_e::OAM_SCAN};
        std::uint32_t dots_in_mode{0};
        bool frame_ready{false};
        bool lcd_was_off{true};
        std::array<std::uint32_t, gb::LCD_WIDTH * gb::LCD_HEIGHT> fb{};
        // BG / window pixels (color index + fetch-time attribute) for the line
        // currently being drawn. render_bg_scanline + render_window_scanline
        // write it; render_sprites_scanline reads color_index to honor the OBJ
        // BG-priority bit (OBJ behind BG colors 1-3) and will eventually read
        // attr.priority for the CGB BG-over-OBJ master override. On DMG every
        // entry's attr is the DMG_BG_ATTR default — color_index is the only
        // varying field today.
        std::array<bg_pixel, gb::LCD_WIDTH> bg_attr_line{};
        // Window per-frame latch: once LY hits WY in a frame the window is armed
        // until the next frame boundary, even if LCDC.5 toggles in between.
        bool window_triggered{false};
        // Window's own line counter: advances only on scanlines where at least
        // one window pixel was actually drawn. Not the same as LY-WY.
        std::uint8_t window_line{0};

        // State machine
        void advance(std::uint32_t t_cycles);
        std::uint32_t dots_for(mode_e mode) const;
        void enter_oam_scan();
        void enter_drawing();
        void enter_hblank();
        void enter_vblank();
        void next_line();

        // Helpers
        void render_bg_scanline(std::uint8_t ly);
        void render_window_scanline(std::uint8_t ly);
        void render_sprites_scanline(std::uint8_t ly);
        void set_stat_mode(mode_e new_mode);
        void update_lyc_coincidence();
    };
} // namespace gbemu
