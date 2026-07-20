#pragma once
#ifndef _H_HDMA_H_
#    define _H_HDMA_H_

#    include <cstdint>

namespace gbemu {
    struct mmu;
    struct cpu;
    struct ppu;

    // CGB-only HDMA / GDMA controller ($FF51-$FF55).
    //
    // HDMA1/2 latch the source high/low bytes (low 4 bits of HDMA2 are dropped
    // — transfers are 16-byte aligned).  HDMA3/4 latch the destination
    // high/low bytes; the destination is always inside VRAM ($8000-$9FF0), so
    // HDMA3 is masked into $80-$9F and writes route through the current VBK
    // bank.  HDMA5 is the trigger/status register:
    //
    //   write bit 7 = 0 → General Purpose DMA (blocking).  Copies the whole
    //                     length in one shot and ticks the CPU through it,
    //                     so PPU/APU/timer keep advancing.  When done,
    //                     HDMA5 reads back as $FF.
    //   write bit 7 = 1 → H-Blank DMA.  Copies 16 bytes on every PPU
    //                     enter_hblank edge until the length is exhausted;
    //                     HDMA5 reads back as (remaining_blocks - 1) with
    //                     bit 7 = 0 while running.
    //   write bit 7 = 0 while an H-Blank DMA is running → terminate it;
    //                     HDMA5 reads back as $80 | (remaining_blocks - 1).
    //
    // On DMG the subsystem stays inert: it never registers its observers
    // because the core only constructs it under cgb_mode (TODO: it's
    // constructed unconditionally today but every public entrypoint short-
    // circuits when mmu.cgb_mode() is false).
    struct hdma {
        hdma(mmu& m, cpu& c);

        // Called from a hook installed on the PPU by core::core().  When an
        // H-Blank DMA is in flight, copies one 16-byte block and ticks the
        // CPU through 32 T-cycles (CPU clock — the cost in both DMG and CGB
        // double-speed; the tick callback splits frequency domains for
        // PPU/APU).
        void on_hblank();

        // True while an H-Blank DMA is in flight.  Surfaced for the MMIO
        // read mask so HDMA5 returns the live status.
        bool hblank_running() const { return hblank_running_; }

        // Number of blocks left (each block = 16 bytes) and the
        // canonicalised HDMA5 readback byte: bit 7 = 0 while running, bit 7
        // = 1 when idle/done; bits 0-6 = (remaining - 1) when running or
        // 0x7F otherwise (so the full byte is $FF when idle, $80 | (n-1)
        // after a terminate).
        std::uint8_t status_byte() const;

        // Reset wipes runtime state (HDMA5 storage is wiped by mmu::reset()
        // separately).  Called from core::reset().
        void reset();

    private:
        mmu& mmu_;
        cpu& cpu_;

        // Active transfer state.  Source/dest are pre-masked at HDMA5 write
        // time; src_ wraps inside its original bank, dst_ wraps inside VRAM
        // (the dest is always in $8000-$9FF0).
        std::uint16_t src_{0};
        std::uint16_t dst_{0};
        // Blocks left to copy (each block = 16 bytes).  Zero means idle.
        std::uint8_t blocks_remaining_{0};
        // True while an H-Blank DMA is armed; false during/after a GP DMA
        // and after a terminate.
        bool hblank_running_{false};

        void on_hdma5_write(std::uint8_t v);
        void copy_block();
    };
} // namespace gbemu

#endif // _H_HDMA_H_
