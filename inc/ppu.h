#pragma once
#ifndef _H_PPU_H_
#    define _H_PPU_H_

#    include <array>
#    include <cstdint>

#    include <mmu.h>

namespace gbemu {
    struct ppu {
        // PPU mode — underlying values match bits 0-1 of the STAT register,
        // so writing the mode to STAT is just `(stat & 0xFC) | uint8_t(mode)`.
        enum class mode_e : std::uint8_t {
            HBLANK = 0,
            VBLANK = 1,
            OAM_SCAN = 2,
            DRAWING = 3,
        };

        explicit ppu(mmu& m);
        void step(std::uint32_t t_cycles);
        const std::uint32_t* framebuffer() const { return fb.data(); }
        bool consume_frame_ready(); // edge-trigger SDL present
    private:
        // Per-scanline timing in dots. Drawing length is fixed; on real hardware
        // it varies with sprite/window content, but we don't model that yet.
        static constexpr std::uint32_t OAM_SCAN_DOTS = 80;
        static constexpr std::uint32_t DRAWING_DOTS = 172;
        static constexpr std::uint32_t HBLANK_DOTS = 204;
        static constexpr std::uint32_t SCANLINE_DOTS = OAM_SCAN_DOTS + DRAWING_DOTS + HBLANK_DOTS; // 456
        static constexpr std::uint8_t VISIBLE_LINES = 144;
        static constexpr std::uint8_t TOTAL_LINES = 154;

        mmu& m;
        mode_e mode{mode_e::OAM_SCAN};
        std::uint32_t dots_in_mode{0};
        bool frame_ready{false};
        bool lcd_was_off{true};
        std::array<std::uint32_t, 160 * 144> fb{};
        // BG color indices (pre-palette, 0..3) for the line currently being drawn.
        // render_bg_scanline writes it; render_sprites_scanline reads it to honor
        // the per-sprite BG-priority bit (OBJ behind BG colors 1-3).
        std::array<std::uint8_t, 160> bg_color_line{};
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
        void request_irq(std::uint8_t bit);
    };
} // namespace gbemu

#endif /* _H_PPU_H_ */
