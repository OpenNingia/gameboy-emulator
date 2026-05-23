#include <cstdio>

#include <mmu.h>
#include <serial.h>

using namespace gbemu;

serial::serial(mmu& m) : mmu_(m) {
    m.set_mmio_write_handler(0xFF02, [this](std::uint8_t v) { on_sc_write(v); });
}

void serial::on_sc_write(std::uint8_t val) {
    // Transfer requested with internal clock: emit SB to stdout, clear bit 7.
    if (val == 0x81) {
        std::putchar(static_cast<char>(mmu_.hwr_sb()));
        std::fflush(stdout);
        mmu_.hwr_sc(0x01);
        // TODO: raise IF.3 (serial interrupt) once timing is modeled.
    }
}
