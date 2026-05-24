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
              pc_idx(0) {}

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