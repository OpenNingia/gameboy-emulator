#include <cstring>
#include <span>

#include <core.h>
#include <exc.hpp>
#include <log.h>
#include <mbc.h>
#include <opcodes.hpp>

using namespace gbemu;

void core::load(rom_file& c) {
    // Hand the ROM bytes off to a freshly-built MBC; the MMU keeps the
    // owning pointer and routes cartridge accesses through it.
    auto cart = make_mbc(std::move(c.data));
    // Latch the hardware-model selection from the cartridge's CGB flag
    // ($0143) before the MBC moves into the MMU. Foothold for the upcoming
    // CGB refactors — no consumer reads cgb_mode yet on DMG-only carts.
    cgb_mode = classify_cgb_flag(cart->cgb_flag()) != cgb_support::none;
    mmu.attach_cartridge(std::move(cart));
    // Mirror the model selection onto the MMU so its MMIO read masks and the
    // VBK/SVBK/KEY1 write handlers know whether to act as DMG or CGB hardware.
    mmu.set_cgb_mode(cgb_mode);

    // reset registers
    memset(&regs, 0, sizeof(regs));
}

void core::load(bios_file const& c) {
    // Hand the BIOS image to the MMU, which both copies the bytes and arms
    // the $0000-$00FF overlay so the next reads see BIOS rather than the
    // cartridge's first bank.
    mmu.load_bios(std::span<const std::uint8_t>{c.data.data(), c.data.size()});

    // reset registers
    memset(&regs, 0, sizeof(regs));
}

void core::init() {
    if (mmu.bios_active()) {
        regs.af.u16 = 0x0000;
        regs.bc.u16 = 0x0000;
        regs.de.u16 = 0x0000;
        regs.hl.u16 = 0x0000;
        regs.sp = 0xFFFE;
        regs.pc = 0x00;
    } else {
        // initial registry values
        mmu.initialize_registers();

        if (cgb_mode) {
            // Post-CGB-BIOS snapshot: A=$11 is the CGB signature games
            // check at boot (Pokémon Crystal hangs on a black screen
            // without it because the CGB palette init path keys off
            // A==$11 to seed BCPD); F=$80 carries the carry bit a real
            // CGB leaves set after the logo check.  DE=$FF56 and
            // HL=$000D mirror the values the actual boot ROM leaves
            // behind.
            regs.af.u16 = 0x1180;
            regs.bc.u16 = 0x0000;
            regs.de.u16 = 0xFF56;
            regs.hl.u16 = 0x000D;
        } else {
            regs.af.u16 = 0x01B0;
            regs.bc.u16 = 0x0013;
            regs.de.u16 = 0x00D8;
            regs.hl.u16 = 0x014D;
        }
        regs.sp = 0xFFFE;
        regs.pc = 0x100;
    }
}

void core::reset() {
    cpu.reset();
    std::memset(&regs, 0, sizeof(regs));
    mmu.reset();
    ppu.reset();
    timer.reset();
    apu.reset();
    hdma.reset();
    pc_ring.fill(0);
    pc_idx = 0;
    total_cycles = 0;
    init();
}

std::uint32_t core::step() {
    pc_ring[pc_idx] = regs.pc;
    pc_idx = (pc_idx + 1) % pc_ring.size();
    auto saved_pc = regs.pc;

    try {
        // APU / timer / total_cycles tick at M-cycle granularity via the
        // tick callback installed in core::core(); PPU ticks accumulate in
        // pending_ppu_t and are flushed here in one bulk step so the PPU
        // sees CPU memory writes and its own mode transitions in the same
        // relative order it would on the per-instruction model.  See the
        // callback comment in core.h for why.
        auto total = static_cast<std::uint32_t>(cpu.step());
        if (irq.dispatch()) {
            total += 20;
        }
        // Drain pending_ppu_t in a loop so any cpu.tick() that lands inside
        // ppu.step() (notably the HBlank DMA block copy fired from
        // ppu::enter_hblank → hdma::on_hblank) is picked up by the next
        // iteration instead of being silently zeroed out below.
        while (pending_ppu_t) {
            const auto t = pending_ppu_t;
            pending_ppu_t = 0;
            ppu.step(t);
        }
        return total;
    } catch (const gbemu_exception& e) {
        LOG_ERROR(gbemu::log::root(), "@PC={:04x} AF={:04x} BC={:04x} DE={:04x} HL={:04x} SP={:04x} : {}", saved_pc,
                  regs.af.u16, regs.bc.u16, regs.de.u16, regs.hl.u16, regs.sp, e.what());
        throw;
    }
}
