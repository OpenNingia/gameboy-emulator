#pragma once

#include <array>
#include <cstdint>

#include <gb_layout.h>
#include <mmu.h>

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
    private:
        mmu& mmu_;
        irq& irq_;
        mode_e mode{mode_e::OAM_SCAN};
        std::uint32_t dots_in_mode{0};
        bool frame_ready{false};
        bool lcd_was_off{true};
        std::array<std::uint32_t, gb::LCD_WIDTH * gb::LCD_HEIGHT> fb{};
        // BG color indices (pre-palette, 0..3) for the line currently being drawn.
        // render_bg_scanline writes it; render_sprites_scanline reads it to honor
        // the per-sprite BG-priority bit (OBJ behind BG colors 1-3).
        std::array<std::uint8_t, gb::LCD_WIDTH> bg_color_line{};
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
