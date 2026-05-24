#include <gb_layout.h>
#include <irq.h>
#include <timer.h>

using namespace gbemu;

timer::timer(mmu& m, irq& i) : mmu_(m), irq_(i) {
    m.add_mmio_write_handler(gb::io::DIV, [this](std::uint8_t v) { div_trigger(v); });
}

void timer::div_trigger(std::uint8_t /*val*/) {
    // Writing any value to DIV resets it to 0 (hardware behavior).
    // io_store bypasses write_u8's handler fan-out so we don't recurse
    // back into this handler.
    mmu_.io_store(gb::io::DIV, 0);
    div_cnt = 0;
}

void timer::step(std::uint32_t cycles) {
    div_cnt += cycles;
    while (div_cnt >= gb::DIV_TICK_CYCLES) {
        div_cnt -= gb::DIV_TICK_CYCLES;
        // io_store bypasses write_u8 so our own DIV handler (which models
        // "ROM writes DIV -> reset to 0") doesn't see every internal
        // increment as a ROM-driven write. Before this, the handler zeroed
        // DIV on every increment and the counter never actually advanced.
        mmu_.io_store(gb::io::DIV, static_cast<std::uint8_t>(mmu_.io_read(gb::io::DIV) + 1));
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
            irq_.request(irq::source::timer);
        } else {
            mmu_.hwr_tima(static_cast<std::uint8_t>(tima));
        }
    }
}
