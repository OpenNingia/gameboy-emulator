#include <dma.h>
#include <mmu.h>

using namespace gbemu;

dma::dma(mmu& m) : mmu_(m) {
    m.set_mmio_write_handler(0xFF46, [this](std::uint8_t v) { trigger(v); });
}

void dma::trigger(std::uint8_t val) {
    // OAM DMA: copy 160 bytes from (val * 0x100) to OAM.
    std::uint16_t src = static_cast<std::uint16_t>(val) << 8;
    for (std::size_t i = 0; i < mmu_.sram.size(); ++i) {
        mmu_.sram[i] = mmu_.read_u8(src + i);
    }
}
