#include <bit>

#include <irq.h>

using namespace gbemu;

bool irq::dispatch() {
    auto pending = mmu.hwr_if() & mmu.hwr_ie() & 0x1F;
    if (cpu.interrupt_enabled && pending) {
        int b = std::countr_zero(static_cast<unsigned>(pending));
        mmu.hwr_if(mmu.hwr_if() & ~(1 << b)); // clear IF bit
        cpu.interrupt_enabled = false;
        cpu.push(cpu.regs.pc);
        cpu.regs.pc = 0x40 + b * 8;
        return true;
    }
    return false;
}
