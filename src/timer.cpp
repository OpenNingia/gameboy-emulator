#include <timer.h>

using namespace gbemu;

constexpr std::uint16_t DIV_ADDR = 0xFF04;
constexpr std::uint16_t TIMA_ADDR = 0xFF05;

timer::timer(mmu& m) : mmu_(m) {
    m.add_mmio_write_handler(DIV_ADDR, [this](std::uint8_t v) { div_trigger(v); });
}

void timer::div_trigger(std::uint8_t val) {
    // writing to DIV resets it to 0
    // mmu_.hwr_div(0); <-- non uso hwr_div(0) per non passare nuovamente da write_u8 e scatenare una ricorsione
    mmu_.mmio[4] = 0;
    div_cnt = 0;
}

void timer::step(std::uint32_t cycles) {
    div_cnt += cycles;
    while (div_cnt >= 256) {
        div_cnt -= 256;
        // Direct mmio update — bypasses write_u8 so registered $FF04 handlers
        // (which model the "ROM writes DIV → reset to 0" hardware behavior)
        // don't see every internal timer tick as a ROM-driven write. Before
        // this, our own div_trigger handler was zeroing mmio[4] on every
        // increment and DIV never actually counted.
        mmu_.mmio[4]++;
    }

    auto tac = mmu_.hwr_tac();

    if (tac & 0x04) {
        // timer enabled
        static constexpr std::uint16_t tima_freqs[] = {1024, 16, 64, 256};
        auto freq = tima_freqs[tac & 0x03];
        tima_cnt += cycles;
        while (tima_cnt >= freq) {
            tima_cnt -= freq;
            auto tima = mmu_.hwr_tima() + 1;
            if (tima > 0xFF) {
                mmu_.hwr_tima(mmu_.hwr_tma());
                mmu_.hwr_if(mmu_.hwr_if() | 0x04); // request timer interrupt
            } else {
                mmu_.hwr_tima(tima);
            }
        }
    }
}
