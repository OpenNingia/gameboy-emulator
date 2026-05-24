#include <gb_layout.h>
#include <timer.h>

using namespace gbemu;

timer::timer(mmu& m) : mmu_(m) {
    m.add_mmio_write_handler(gb::io::DIV, [this](std::uint8_t v) { div_trigger(v); });
}

void timer::div_trigger(std::uint8_t /*val*/) {
    // Writing any value to DIV resets it to 0 (hardware behavior).
    // We poke mmio[] directly to avoid re-entering write_u8 and recursing
    // back into this handler.
    mmu_.mmio[gb::io_offset(gb::io::DIV)] = 0;
    div_cnt = 0;
}

void timer::step(std::uint32_t cycles) {
    div_cnt += cycles;
    while (div_cnt >= gb::DIV_TICK_CYCLES) {
        div_cnt -= gb::DIV_TICK_CYCLES;
        // Direct mmio update — bypasses write_u8 so our own DIV handler
        // (which models "ROM writes DIV -> reset to 0") doesn't see every
        // internal increment as a ROM-driven write. Before this, the handler
        // zeroed mmio[DIV] on every increment and DIV never actually counted.
        mmu_.mmio[gb::io_offset(gb::io::DIV)]++;
    }

    const auto tac = mmu_.hwr_tac();
    if (!(tac & gb::tac::enable))
        return;

    const auto period = gb::tac::tima_period_cycles[tac & gb::tac::clock_select_mask];
    tima_cnt += cycles;
    while (tima_cnt >= period) {
        tima_cnt -= period;
        const auto tima = mmu_.hwr_tima() + 1;
        if (tima > 0xFF) {
            mmu_.hwr_tima(mmu_.hwr_tma());
            mmu_.hwr_if(mmu_.hwr_if() | gb::irq_bit::timer);
        } else {
            mmu_.hwr_tima(static_cast<std::uint8_t>(tima));
        }
    }
}
