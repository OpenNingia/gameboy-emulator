#include <gb_layout.h>
#include <irq.h>
#include <mmu.h>
#include <serial.h>

using namespace gbemu;

serial::serial(mmu& m, irq& i) : mmu_(m), irq_(i) {
    m.add_mmio_write_handler(gb::io::SC, [this](std::uint8_t v) { on_sc_write(v); });
}

void serial::on_sc_write(std::uint8_t val) {
    // Transfer requested with internal clock: complete it immediately by
    // clearing SC bit 7 (transfer-in-progress flag) and raising IF.3
    // (serial-complete interrupt).
    //
    // Real hardware takes ~1024 T-cycles to shift out a byte, but ROMs that
    // wait on the transfer (Blargg's combined cpu_instrs runner writes SC=$81
    // and then HALTs expecting IF.3) only care that the bit eventually
    // arrives.  Raising it eagerly is safe as long as the CPU can't observe
    // the missing delay — ROMs that don't enable IE.3 just see the IF flag,
    // which is exactly how the HALT-bug check on DMG already behaves.
    //
    // The actual byte being transmitted (SB) is observed by separate handlers
    // registered on $FF02 (e.g. the Application's stdout-echo handler, the
    // debugger's serial ring buffer).  Both gate on val == 0x81 the same way
    // this function does.
    // val == 0x81: bit 7 = transfer start, bit 0 = internal clock source.
    if (val == 0x81) {
        mmu_.hwr_sc(0x01); // clear bit 7 (transfer complete), leave clock-source bit set
        irq_.request(irq::source::serial);
    }
}
