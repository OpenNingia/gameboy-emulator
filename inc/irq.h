#pragma once
#ifndef _H_IRQ_H_
#    define _H_IRQ_H_

#    include <cstdint>

#    include <cpu.h>
#    include <mmu.h>

namespace gbemu {
    struct irq {
        // Interrupt sources, encoded as IF/IE bit indices so the dispatcher's
        // `0x40 + index * 8` vector math falls out for free. Order matches
        // the DMG priority: vblank wins ties, joypad loses them.
        enum class source : std::uint8_t {
            vblank = 0,
            lcd_stat = 1,
            timer = 2,
            serial = 3,
            joypad = 4,
        };

        irq(cpu& c, mmu& m) : cpu(c), mmu(m) {}

        // Raise an interrupt: ORs the bit for `s` into IF. Subsystems use
        // this instead of poking IF directly so the IRQ source is named at
        // the call site and the bit-position knowledge stays inside irq.
        void request(source s);

        // returns true if an interrupt was dispatched
        bool dispatch();

    private:
        cpu& cpu;
        mmu& mmu;
    };
} // namespace gbemu

#endif // _H_IRQ_H_
