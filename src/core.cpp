#include <cstdio>
#include <string>

#include <core.h>
#include <exc.hpp>
#include <log.h>
#include <mbc.h>
#include <opcodes.hpp>

using namespace gbemu;

void core::load(rom_file& c) {
    // Hand the ROM bytes off to a freshly-built MBC; the MMU keeps the
    // owning pointer and routes cartridge accesses through it.
    mmu.attach_cartridge(make_mbc(std::move(c.data)));

    // reset registers
    memset(&regs, 0, sizeof(regs));
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

void core::dump_pc_ring() const {
    // Walk the ring oldest -> newest.  pc_idx is where the *next* PC would
    // be written, so it doubles as the position of the oldest entry.
    LOG_INFO(log::root(), "=== PC ring dump (most recent first) ===");
    constexpr std::size_t cols = 8;
    std::string line;
    line.reserve(cols * 6 + 4);
    for (std::size_t i = 0; i < pc_ring.size(); ++i) {
        std::size_t k = (pc_idx + pc_ring.size() - 1 - i) % pc_ring.size();
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%04X ", pc_ring[k]);
        line += buf;
        if ((i + 1) % cols == 0) {
            LOG_INFO(log::root(), "{}", line);
            line.clear();
        }
    }
    if (!line.empty())
        LOG_INFO(log::root(), "{}", line);
    LOG_INFO(log::root(), "AF={:04x} BC={:04x} DE={:04x} HL={:04x} SP={:04x} PC={:04x} IF={:02x} IE={:02x} IME={}",
             regs.af.u16, regs.bc.u16, regs.de.u16, regs.hl.u16, regs.sp, regs.pc, mmu.hwr_if(), mmu.hwr_ie(),
             cpu.interrupt_enabled ? 1 : 0);
}

std::uint32_t core::step() {
    pc_ring[pc_idx] = regs.pc;
    pc_idx = (pc_idx + 1) % pc_ring.size();
    auto saved_pc = regs.pc;

    try {
        auto total = cpu.step();

        if (irq.dispatch()) {
            total += 20;
        }

        ppu.step(total);
        timer.step(total);

        total_cycles += total;

        return total;
    } catch (const gbemu_exception& e) {
        LOG_ERROR(gbemu::log::root(), "@PC={:04x} AF={:04x} BC={:04x} DE={:04x} HL={:04x} SP={:04x} : {}", saved_pc,
                  regs.af.u16, regs.bc.u16, regs.de.u16, regs.hl.u16, regs.sp, e.what());
        throw;
    }
}
