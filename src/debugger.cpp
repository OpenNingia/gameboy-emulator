#include <algorithm>
#include <bit>
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

step_result debugger::step() {
    step_result r{};
    r.cycles = core_.step();
    if (!watchpoints_.empty()) {
        std::uint16_t hit_addr = 0;
        if (sample_watchpoints(hit_addr)) {
            r.watchpoint_hit = true;
            r.watchpoint_addr = hit_addr;
        }
    }
    return r;
}

run_result debugger::run_until(const stop_condition& cond, std::uint64_t max_cycles) {
    run_result r{};
    std::size_t serial_scan_pos = 0;

    while (r.cycles_consumed < max_cycles) {
        const auto sr = step();
        r.cycles_consumed += sr.cycles;
        ++r.instructions;

        if (sr.watchpoint_hit) {
            r.outcome = run_outcome::watchpoint;
            r.hit_addr = sr.watchpoint_addr;
            return r;
        }
        if (breakpoint_has(core_.regs.pc)) {
            r.outcome = run_outcome::breakpoint;
            r.hit_addr = core_.regs.pc;
            return r;
        }

        switch (cond.kind) {
            case stop_kind::none:
                break;
            case stop_kind::pc_eq:
                if (core_.regs.pc == static_cast<std::uint16_t>(cond.value)) {
                    r.outcome = run_outcome::condition;
                    r.hit_addr = core_.regs.pc;
                    return r;
                }
                break;
            case stop_kind::cycles_ge:
                if (core_.total_cycles >= cond.value) {
                    r.outcome = run_outcome::condition;
                    return r;
                }
                break;
            case stop_kind::serial_match: {
                // Skip if the buffer hasn't grown since the last scan.  When
                // it has, search the new portion (with a `match_size - 1`
                // pre-window so a match that straddles the previous boundary
                // is still found).
                const auto& sm = cond.serial_match;
                if (sm.empty() || serial_buf_.size() <= serial_scan_pos)
                    break;
                const std::size_t start = (serial_scan_pos >= sm.size()) ? (serial_scan_pos - sm.size() + 1) : 0;
                const auto it = std::search(serial_buf_.begin() + static_cast<std::ptrdiff_t>(start), serial_buf_.end(),
                                            sm.begin(), sm.end());
                if (it != serial_buf_.end()) {
                    r.outcome = run_outcome::condition;
                    return r;
                }
                serial_scan_pos = serial_buf_.size();
                break;
            }
            case stop_kind::vblank:
                if (core_.ppu.consume_frame_ready()) {
                    r.outcome = run_outcome::condition;
                    return r;
                }
                break;
            case stop_kind::instr_count:
                if (r.instructions >= cond.value) {
                    r.outcome = run_outcome::condition;
                    return r;
                }
                break;
        }
    }

    r.outcome = run_outcome::max_cycles_safety;
    return r;
}

std::uint64_t debugger::total_cycles() const {
    return core_.total_cycles;
}

std::uint16_t debugger::current_pc() const {
    return core_.regs.pc;
}

void debugger::breakpoint_set(std::uint16_t addr) {
    bp_bitmap_[addr >> 3] |= static_cast<std::uint8_t>(1u << (addr & 7));
}

void debugger::breakpoint_clear(std::uint16_t addr) {
    bp_bitmap_[addr >> 3] &= static_cast<std::uint8_t>(~(1u << (addr & 7)));
}

bool debugger::breakpoint_has(std::uint16_t addr) const {
    return (bp_bitmap_[addr >> 3] & (1u << (addr & 7))) != 0;
}

std::vector<std::uint16_t> debugger::breakpoint_list() const {
    std::vector<std::uint16_t> r;
    for (std::size_t i = 0; i < bp_bitmap_.size(); ++i) {
        auto b = bp_bitmap_[i];
        while (b) {
            const int bit = std::countr_zero(b);
            r.push_back(static_cast<std::uint16_t>((i << 3) | static_cast<std::uint32_t>(bit)));
            b &= static_cast<std::uint8_t>(b - 1);
        }
    }
    return r;
}

void debugger::watchpoint_set(std::uint16_t addr, std::uint16_t len) {
    if (len == 0)
        return;
    watchpoint_clear(addr);
    watchpoint w{addr, len, {}};
    w.last.reserve(len);
    for (std::uint16_t i = 0; i < len; ++i)
        w.last.push_back(core_.mmu.read_u8(static_cast<std::uint16_t>(addr + i)));
    watchpoints_.push_back(std::move(w));
}

void debugger::watchpoint_clear(std::uint16_t addr) {
    watchpoints_.erase(
        std::remove_if(watchpoints_.begin(), watchpoints_.end(), [&](const watchpoint& w) { return w.addr == addr; }),
        watchpoints_.end());
}

void debugger::watchpoint_clear_all() {
    watchpoints_.clear();
}

bool debugger::sample_watchpoints(std::uint16_t& out_addr) {
    bool any = false;
    for (auto& w : watchpoints_) {
        bool changed = false;
        for (std::uint16_t i = 0; i < w.len; ++i) {
            const auto cur = core_.mmu.read_u8(static_cast<std::uint16_t>(w.addr + i));
            if (cur != w.last[i]) {
                w.last[i] = cur;
                changed = true;
            }
        }
        if (changed && !any) {
            out_addr = w.addr;
            any = true;
        }
    }
    return any;
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

void debugger::dump_ppu(std::ostream& os) const {
    const auto lcdc = core_.mmu.hwr_lcdc();
    const auto stat = core_.mmu.hwr_stat();
    const auto scy = core_.mmu.hwr_scy();
    const auto scx = core_.mmu.hwr_scx();
    const auto ly = core_.mmu.hwr_ly();
    const auto lyc = core_.mmu.hwr_lyc();
    const auto bgp = core_.mmu.hwr_bgp();
    const auto obp0 = core_.mmu.hwr_obp0();
    const auto obp1 = core_.mmu.hwr_obp1();
    const auto wy = core_.mmu.hwr_wy();
    const auto wx = core_.mmu.hwr_wx();

    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "LCDC=%02X STAT=%02X SCY=%02X SCX=%02X LY=%02X LYC=%02X "
                  "BGP=%02X OBP0=%02X OBP1=%02X WY=%02X WX=%02X\n",
                  lcdc, stat, scy, scx, ly, lyc, bgp, obp0, obp1, wy, wx);
    os << buf;
    std::snprintf(buf, sizeof(buf),
                  "LCDC: enable=%d win_map=%d win_en=%d tile_data=%d bg_map=%d obj_size=%d obj_en=%d bg_en=%d\n",
                  (lcdc >> 7) & 1, (lcdc >> 6) & 1, (lcdc >> 5) & 1, (lcdc >> 4) & 1, (lcdc >> 3) & 1, (lcdc >> 2) & 1,
                  (lcdc >> 1) & 1, lcdc & 1);
    os << buf;
    std::snprintf(buf, sizeof(buf), "STAT: lyc_ie=%d mode2_ie=%d mode1_ie=%d mode0_ie=%d coincidence=%d mode=%d\n",
                  (stat >> 6) & 1, (stat >> 5) & 1, (stat >> 4) & 1, (stat >> 3) & 1, (stat >> 2) & 1, stat & 3);
    os << buf;
}

void debugger::dump_mbc(std::ostream& os) const {
    // PR5 will add mbc::debug_state() for bank/RAM-enable info.  For PR2 we
    // just expose the cartridge header bytes that classify the chip.
    const auto cart_type = core_.mmu.read_u8(0x0147);
    const auto rom_size = core_.mmu.read_u8(0x0148);
    const auto ram_size = core_.mmu.read_u8(0x0149);

    char buf[128];
    std::snprintf(buf, sizeof(buf), "cart_type=$%02X rom_size=$%02X ram_size=$%02X\n", cart_type, rom_size, ram_size);
    os << buf;
    os << "bank_state=(PR5)\n";
}

void debugger::dump_stack(std::ostream& os, std::size_t n) const {
    char buf[24];
    for (std::size_t i = 0; i < n; ++i) {
        const auto sp_i = static_cast<std::uint16_t>(core_.regs.sp + i * 2);
        const auto lo = core_.mmu.read_u8(sp_i);
        const auto hi = core_.mmu.read_u8(static_cast<std::uint16_t>(sp_i + 1));
        const auto word = static_cast<std::uint16_t>(lo | (hi << 8));
        std::snprintf(buf, sizeof(buf), "%04X: %04X\n", sp_i, word);
        os << buf;
    }
}
