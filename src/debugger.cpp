#include <algorithm>
#include <cstdio>
#include <ostream>
#include <string>

#include <core.h>
#include <debugger.h>
#include <disasm.h>

using namespace gbemu;

debugger::debugger(core& c) : core_(c) {
    c.mmu.add_mmio_write_handler(0xFF02, [this](std::uint8_t v) {
        if (v == 0x81) {
            serial_buf_.push_back(static_cast<char>(core_.mmu.hwr_sb()));
        }
    });
}

std::uint32_t debugger::step() {
    return core_.step();
}

std::uint64_t debugger::total_cycles() const {
    return core_.total_cycles;
}

void debugger::dump_regs(std::ostream& os) const {
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "AF=%04X BC=%04X DE=%04X HL=%04X SP=%04X PC=%04X\n"
                  "Z=%d N=%d H=%d C=%d IME=%d HALT=%d STOP=%d\n",
                  core_.regs.af.u16, core_.regs.bc.u16, core_.regs.de.u16, core_.regs.hl.u16, core_.regs.sp,
                  core_.regs.pc, core_.regs.z_flag() ? 1 : 0, core_.regs.n_flag() ? 1 : 0, core_.regs.h_flag() ? 1 : 0,
                  core_.regs.c_flag() ? 1 : 0, core_.cpu.interrupt_enabled ? 1 : 0, core_.cpu.halted ? 1 : 0,
                  core_.cpu.stopped ? 1 : 0);
    os << buf;
}

void debugger::dump_mem(std::ostream& os, std::uint16_t addr, std::uint16_t len) const {
    char buf[16];
    for (std::uint32_t i = 0; i < len; i += 16) {
        const auto cnt = std::min<std::uint32_t>(16, len - i);
        std::snprintf(buf, sizeof(buf), "%04X:", static_cast<std::uint16_t>(addr + i));
        os << buf;
        for (std::uint32_t j = 0; j < cnt; ++j) {
            std::snprintf(buf, sizeof(buf), " %02X", core_.mmu.read_u8(static_cast<std::uint16_t>(addr + i + j)));
            os << buf;
        }
        os << "\n";
    }
}

void debugger::dump_pc_ring(std::ostream& os, std::size_t n) const {
    constexpr std::size_t cols = 8;
    const auto& ring = core_.pc_ring;
    n = std::min(n, ring.size());

    char buf[8];
    std::string line;
    line.reserve(cols * 5 + 1);
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t k = (core_.pc_idx + ring.size() - 1 - i) % ring.size();
        std::snprintf(buf, sizeof(buf), "%04X ", ring[k]);
        line += buf;
        if ((i + 1) % cols == 0) {
            os << line << "\n";
            line.clear();
        }
    }
    if (!line.empty())
        os << line << "\n";
}

void debugger::disasm(std::ostream& os, std::uint16_t addr, std::size_t n) const {
    char buf[16];
    std::uint16_t cur = addr;
    for (std::size_t i = 0; i < n; ++i) {
        const auto r = disasm_one(cur, core_.mmu);
        std::snprintf(buf, sizeof(buf), "%04X: ", cur);
        os << buf;
        // Emit up to 3 raw bytes (length significant), padding shorter
        // instructions for column alignment with longer ones.
        for (std::uint8_t j = 0; j < 3; ++j) {
            if (j < r.length) {
                std::snprintf(buf, sizeof(buf), "%02X ", r.bytes[j]);
                os << buf;
            } else {
                os << "   ";
            }
        }
        os << " " << r.text << "\n";

        if (r.length == 0)
            break; // safety: malformed table entry
        cur = static_cast<std::uint16_t>(cur + r.length);
    }
}
