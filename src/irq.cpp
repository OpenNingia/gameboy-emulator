#include <bit>

#include <gb_layout.h>
#include <irq.h>

using namespace gbemu;

void irq::request(source s) {
    const auto bit = static_cast<std::uint8_t>(1u << static_cast<std::uint8_t>(s));
    mmu.hwr_if(static_cast<std::uint8_t>(mmu.hwr_if() | bit));
}

bool irq::dispatch() {
    const auto pending = static_cast<std::uint8_t>(mmu.hwr_if() & mmu.hwr_ie() & gb::irq_bit::all_mask);
    if (cpu.interrupt_enabled && pending) {
        const int b = std::countr_zero(static_cast<unsigned>(pending));
        // Clear the serviced bit in IF.
        mmu.hwr_if(static_cast<std::uint8_t>(mmu.hwr_if() & ~(1u << b)));
        cpu.interrupt_enabled = false;
        // IRQ servicing is 5 M-cycles (20 T): 2 entry NOPs + 2 push writes
        // + 1 internal jump cycle. push() ticks 8 T (2 bus_writes); we
        // explicitly tick the remaining 12 T so the PPU/APU/timer see the
        // dispatch overhead at roughly the right granularity.
        cpu.tick(8);
        cpu.push(cpu.regs.pc);
        cpu.tick(4);
        cpu.regs.pc = static_cast<std::uint16_t>(gb::IRQ_VECTOR_BASE + b * gb::IRQ_VECTOR_STRIDE);
        return true;
    }
    return false;
}
