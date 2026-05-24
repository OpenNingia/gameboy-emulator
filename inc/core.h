#pragma once
#ifndef _H_CORE_H_
#    define _H_CORE_H_

#    include <cstdint>
#    include <optional>
#    include <string>

#    include <alu.h>
#    include <apu.h>
#    include <card.h>
#    include <cpu.h>
#    include <dma.h>
#    include <irq.h>
#    include <joypad.h>
#    include <mmu.h>
#    include <ppu.h>
#    include <registers.h>
#    include <serial.h>
#    include <timer.h>

namespace gbemu {

    struct core {
        core()
            : cpu(mmu, regs),
              regs(),
              mmu(),
              ppu(mmu, irq),
              irq(cpu, mmu),
              serial(mmu, irq),
              dma(mmu),
              timer(mmu, irq),
              apu(mmu),
              joypad(mmu, irq),
              pc_ring(),
              pc_idx(0) {
            // Wire the CPU's M-cycle tick callback.  APU, timer and
            // total_cycles advance at M-cycle granularity inside the
            // callback; PPU ticks are *accumulated* into pending_ppu_t and
            // flushed in one bulk ppu.step() at the end of core::step().
            //
            // Why split: our PPU renders each scanline atomically when it
            // enters mode 3, so the model it expects from callers is
            // "complete an instruction, then advance the dot counter."
            // Forwarding ticks mid-instruction shifts the relative ordering
            // of CPU memory writes and PPU mode transitions by up to one
            // M-cycle, which breaks games that retune SCX/SCY/LCDC during
            // tight HBlank STAT IRQs (the original symptom: SML status bar
            // flickered by one scanline at irregular intervals).
            //
            // Sub-instruction PPU accuracy (Mooneye PPU suite, FIFO
            // emulation, mid-scanline LCDC tricks) would need a real
            // dot-driven PPU rewrite — see TODO.md §9.
            cpu.tick_ctx = this;
            cpu.tick_fn = [](void* ctx, std::uint8_t t) {
                auto* c = static_cast<core*>(ctx);
                c->pending_ppu_t += t;
                c->apu.step(t);
                c->timer.step(t);
                c->total_cycles += t;
            };
        }

        cpu cpu;
        registers regs;
        mmu mmu;
        ppu ppu;
        irq irq;
        serial serial;
        dma dma;
        timer timer;
        apu apu;
        joypad joypad;

        std::array<std::uint16_t, 256> pc_ring;
        std::size_t pc_idx;
        std::uint64_t total_cycles{0};
        // PPU tick accumulator: bumped from the tick callback during
        // cpu.step() / irq.dispatch(), drained by a single ppu.step() at the
        // end of core::step().  See the tick-callback comment in the ctor
        // for the rationale.
        std::uint32_t pending_ppu_t{0};

        // load a cartridge
        void load(rom_file& c);
        // load bios
        void load(bios_file const& c);
        // init registers
        void init();
        // execute an emulation step
        std::uint32_t step();
    };
} // namespace gbemu

#endif /* _H_CORE_H_ */