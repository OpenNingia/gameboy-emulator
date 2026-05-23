#pragma once
#ifndef _H_IRQ_H_
#define _H_IRQ_H_

#include <cpu.h>
#include <mmu.h>

namespace gbemu {
    struct irq {
        irq(cpu& c, mmu& m) : cpu(c), mmu(m) {}
        
        // returns true if an interrupt was dispatched
        bool dispatch();

        private:
            cpu& cpu;
            mmu& mmu;
    };
}

#endif // _H_IRQ_H_