#include <cpu.h>
#include <gb_layout.h>
#include <hdma.h>
#include <mmu.h>

using namespace gbemu;

namespace {
    constexpr std::uint8_t HDMA_BLOCK_BYTES = 16;
    // 8 M-cycles (32 T at CPU clock) per 16-byte block, the same count in
    // single- and double-speed mode.  The tick callback in core::core()
    // halves what it forwards to PPU/APU in double-speed, so PPU/APU
    // observe the wall-clock-equivalent 16 base-clock T per block when
    // running at 2x.
    constexpr std::uint8_t HDMA_BLOCK_CYCLES = 32;
} // namespace

hdma::hdma(mmu& m, cpu& c) : mmu_(m), cpu_(c) {
    m.add_mmio_write_handler(gb::io::HDMA5, [this](std::uint8_t v) { on_hdma5_write(v); });
    // Seed the HDMA5 storage with the idle status_byte ($FF). The mmu's
    // mmio_ array is value-initialised to 0 at construction; without this
    // the very first read of HDMA5 on CGB would observe 0 (= "running, 0
    // blocks remaining") instead of the expected idle marker.
    mmu_.io_store(gb::io::HDMA5, status_byte());
}

void hdma::reset() {
    src_ = 0;
    dst_ = 0;
    blocks_remaining_ = 0;
    hblank_running_ = false;
    // mmu::reset() has already zeroed mmio_[HDMA5]; sync the readback to
    // $FF so the next program sees "idle, no transfer pending".
    mmu_.io_store(gb::io::HDMA5, status_byte());
}

std::uint8_t hdma::status_byte() const {
    if (blocks_remaining_ == 0) {
        // Idle / completed transfer reads back as $FF (bit 7 = 1 means
        // "no transfer active", bits 0-6 are all 1).
        return 0xFF;
    }
    const std::uint8_t n = static_cast<std::uint8_t>((blocks_remaining_ - 1) & 0x7F);
    // While an H-Blank DMA is armed bit 7 reads 0; after a terminate it
    // reads 1 (the engine is no longer running but blocks_remaining_
    // captures how much was left).
    return hblank_running_ ? n : static_cast<std::uint8_t>(0x80 | n);
}

void hdma::on_hdma5_write(std::uint8_t v) {
    if (!mmu_.cgb_mode())
        return;

    // Writing bit 7 = 0 while an H-Blank DMA is running terminates it;
    // blocks_remaining_ is preserved so status_byte() reports $80 | (n-1).
    if (hblank_running_ && (v & 0x80) == 0) {
        hblank_running_ = false;
        mmu_.io_store(gb::io::HDMA5, status_byte());
        return;
    }

    // Latch the (source, dest, length) triple from the MMIO bytes.  The
    // mask layout matches Pan Docs: HDMA2 drops its low 4 bits; HDMA3 keeps
    // only bits 4-0 and is forced into the $8000-$9FF0 window; HDMA4 drops
    // its low 4 bits.
    const std::uint8_t s_hi = mmu_.io_read(gb::io::HDMA1);
    const std::uint8_t s_lo = static_cast<std::uint8_t>(mmu_.io_read(gb::io::HDMA2) & 0xF0);
    const std::uint8_t d_hi = static_cast<std::uint8_t>((mmu_.io_read(gb::io::HDMA3) & 0x1F) | 0x80);
    const std::uint8_t d_lo = static_cast<std::uint8_t>(mmu_.io_read(gb::io::HDMA4) & 0xF0);
    src_ = static_cast<std::uint16_t>((s_hi << 8) | s_lo);
    dst_ = static_cast<std::uint16_t>((d_hi << 8) | d_lo);
    blocks_remaining_ = static_cast<std::uint8_t>((v & 0x7F) + 1);

    if (v & 0x80) {
        // H-Blank DMA: arm; copy_block fires from on_hblank().
        hblank_running_ = true;
    } else {
        // General Purpose DMA: blocking copy of the whole length now.
        // copy_block ticks the CPU through the 32 T per block so PPU /
        // APU / timer keep advancing.
        hblank_running_ = false;
        while (blocks_remaining_ > 0)
            copy_block();
    }
    mmu_.io_store(gb::io::HDMA5, status_byte());
}

void hdma::on_hblank() {
    if (!hblank_running_ || blocks_remaining_ == 0)
        return;
    copy_block();
    if (blocks_remaining_ == 0)
        hblank_running_ = false;
    mmu_.io_store(gb::io::HDMA5, status_byte());
}

void hdma::copy_block() {
    // 16 bytes from src_ → dst_.  Source goes through mmu.read_u8 so MBC
    // banking is honored; destination writes go through mmu.write_u8 which
    // routes into the current VBK-selected VRAM bank.
    for (std::uint8_t i = 0; i < HDMA_BLOCK_BYTES; ++i) {
        const std::uint8_t byte = mmu_.read_u8(static_cast<std::uint16_t>(src_ + i));
        mmu_.write_u8(static_cast<std::uint16_t>(dst_ + i), byte);
    }
    src_ = static_cast<std::uint16_t>(src_ + HDMA_BLOCK_BYTES);
    dst_ = static_cast<std::uint16_t>(dst_ + HDMA_BLOCK_BYTES);
    --blocks_remaining_;
    // The 32 T-cycle block cost goes through cpu.tick so the PPU / APU /
    // timer all see it.  In double-speed mode the tick callback halves
    // what it forwards to PPU/APU, matching the "twice as fast in 2x"
    // semantic.
    cpu_.tick(HDMA_BLOCK_CYCLES);
}
