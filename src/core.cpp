#include <core.h>
#include <exc.hpp>
#include <log.h>
#include <opcodes.hpp>

using namespace gbemu;

void core::load(rom_file& c) {
    crd = std::move(c);
    // load first two banks
    auto bank1_sz = std::min(crd->data.size(), mmu.rom0.size());
    auto bank2_sz = std::min(crd->data.size() - bank1_sz, mmu.rom1.size());
    memcpy(mmu.rom0.data(), crd->data.data(), bank1_sz);
    memcpy(mmu.rom1.data(), crd->data.data() + bank1_sz, bank2_sz);

    // reset registers
    memset(&regs, 0, sizeof(regs));

    // skip bios
    // mmu.bios_accessible = false;
}

void core::load(bios_file const& c) {
    // load first two banks
    memcpy(mmu.bios.data(), c.data.data(), mmu.bios.size());

    // reset registers
    memset(&regs, 0, sizeof(regs));

    // skip bios
    mmu.bios_accessible = true;
}

void core::init() {
    if (mmu.bios_accessible) {
        regs.af.u16 = 0x0000;
        regs.bc.u16 = 0x0000;
        regs.de.u16 = 0x0000;
        regs.hl.u16 = 0x0000;
        regs.sp = 0xFFFE;
        regs.pc = 0x00;
    } else {
        // initial registry values
        mmu.initialize_registers();

        regs.af.u16 = 0x01B0;
        regs.bc.u16 = 0x0013;
        regs.de.u16 = 0x00D8;
        regs.hl.u16 = 0x014D;
        regs.sp = 0xFFFE;
        regs.pc = 0x100;
    }
}

void core::step() {
    pc_ring[pc_idx] = regs.pc;
    pc_idx = (pc_idx + 1) % pc_ring.size();
    auto saved_pc = regs.pc;

    try {
        auto total = cpu.step();

        if (irq.dispatch()) {
            total += 20;
        }

        ppu.step(total);
    } catch (const gbemu_exception& e) {
        LOG_ERROR(gbemu::log::root(), "@PC={:04x} AF={:04x} BC={:04x} DE={:04x} HL={:04x} SP={:04x} : {}", saved_pc,
                  regs.af.u16, regs.bc.u16, regs.de.u16, regs.hl.u16, regs.sp, e.what());
        throw;
    }
}
