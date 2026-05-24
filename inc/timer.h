#pragma once
#ifndef _H_TIMER_H_
#    define _H_TIMER_H_

#    include <cstdint>

#    include <mmu.h>

namespace gbemu {
    struct irq;

    struct timer {
        timer(mmu& mmu, irq& i);

        // step the timer and return the number of cycles consumed
        void step(std::uint32_t cycles);

        // triggers
        void div_trigger(std::uint8_t val);

        // Clear internal dividers so a Reset starts from a known state.
        void reset() {
            div_cnt = 0;
            tima_cnt = 0;
        }

    private:
        mmu& mmu_;
        irq& irq_;
        std::uint32_t div_cnt{0};
        std::uint32_t tima_cnt{0};
    };
} // namespace gbemu

#endif /* _H_TIMER_H_ */