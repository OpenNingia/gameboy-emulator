#pragma once
#ifndef _H_PPU_H_
#define _H_PPU_H_

#include <cstdint>
#include <array>
#include <mmu.h>

namespace gbemu {
    struct ppu {
        explicit ppu(mmu& m);
        void tick(std::uint32_t t_cycles);
        const std::uint32_t* framebuffer() const { return fb.data(); }
        bool consume_frame_ready();   // edge-trigger SDL present
    private:
        mmu& m;
        std::uint32_t dot_counter { 0 };   // 0..455
        std::uint8_t mode { 2 };          // 2,3,0 visibile; 1 vblank
        bool frame_ready { false };
        std::array<std::uint32_t, 160*144> fb {};
        void render_bg_scanline(std::uint8_t ly);
        void update_stat_mode(std::uint8_t new_mode);
        void update_lyc_coincidence();
        void request_irq(std::uint8_t bit);
    };
}

#endif /* _H_PPU_H_ */