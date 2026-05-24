#include <dma.h>
#include <gb_layout.h>
#include <mmu.h>

using namespace gbemu;

dma::dma(mmu& m) : mmu_(m) {
    m.add_mmio_write_handler(gb::io::DMA, [this](std::uint8_t v) { trigger(v); });
}

void dma::trigger(std::uint8_t val) {
    // OAM DMA: copy 160 bytes from (val * $100) into OAM ($FE00-$FE9F).
    // The high byte of the source address is `val`; the low byte sweeps 00..9F.
    const std::uint16_t src = static_cast<std::uint16_t>(val) << 8;
    for (std::size_t i = 0; i < gb::OAM_TOTAL_BYTES; ++i) {
        mmu_.oam_write(static_cast<std::uint8_t>(i), mmu_.read_u8(static_cast<std::uint16_t>(src + i)));
    }
}
