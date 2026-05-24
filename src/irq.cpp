#include <bit>

#include <gb_layout.h>
#include <irq.h>

using namespace gbemu;

bool irq::dispatch() {
    const auto pending = static_cast<std::uint8_t>(mmu.hwr_if() & mmu.hwr_ie() & gb::irq_bit::all_mask);
    if (cpu.interrupt_enabled && pending) {
        const int b = std::countr_zero(static_cast<unsigned>(pending));
        // Clear the serviced bit in IF.
        mmu.hwr_if(static_cast<std::uint8_t>(mmu.hwr_if() & ~(1u << b)));
        cpu.interrupt_enabled = false;
        cpu.push(cpu.regs.pc);
        cpu.regs.pc = static_cast<std::uint16_t>(gb::IRQ_VECTOR_BASE + b * gb::IRQ_VECTOR_STRIDE);
        return true;
    }
    return false;
}
