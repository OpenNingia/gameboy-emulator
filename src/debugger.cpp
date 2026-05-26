#include <algorithm>
#include <bit>
#include <cstdio>
#include <ostream>
#include <string>

#include <core.h>
#include <debugger.h>
#include <disasm.h>
#include <gb_layout.h>

using namespace gbemu;

debugger::debugger(core& c) : core_(c) {
    c.mmu.add_mmio_write_handler(gb::io::SC, [this](std::uint8_t v) {
        // SC = 0x81: bit 7 = transfer start, bit 0 = internal clock source.
        if (v == 0x81) {
            serial_buf_.push_back(static_cast<char>(core_.mmu.hwr_sb()));
        }
    });
    // Bus-wide write observer — drives watchpoints.  Installing this from
    // the ctor (after core has been constructed) means the post-BIOS
    // initialize_registers() pokes have already run and won't generate
    // spurious watchpoint fires; subsequent core.reset()s, however, will
    // observably fire any watchpoint that covers the reset's writes
    // (acceptable trade — the alternative is suppressing observers around
    // reset, which adds complexity for a corner case).
    c.mmu.add_bus_write_observer([this](std::uint16_t a, std::uint8_t v) { on_bus_write(a, v); });
}

step_result debugger::step() {
    step_result r{};
    // Snapshot the PC *before* the step so the observer can credit any
    // watchpoint write to the originating instruction.  Capturing here
    // (rather than after core_.step()) is load-bearing: by the time the
    // step returns, regs.pc has advanced past the writer (and a CALL/JP/
    // RST/IRQ dispatch may have moved it somewhere completely unrelated).
    pending_writer_pc_ = core_.regs.pc;
    pending_wp_hit_ = false;
    pending_wp_addr_ = 0;
    r.cycles = core_.step();
    if (pending_wp_hit_) {
        r.watchpoint_hit = true;
        r.watchpoint_addr = pending_wp_addr_;
        r.watchpoint_writer_pc = pending_writer_pc_;
    }
    return r;
}

void debugger::on_bus_write(std::uint16_t addr, std::uint8_t val) {
    if (watchpoints_.empty())
        return;
    for (auto& w : watchpoints_) {
        // Range check (handle wrap defensively: `addr + len` can pass 0xFFFF
        // for a watchpoint at the very top of the address space, but len is
        // small in practice so the unsigned compare is enough).
        if (addr < w.addr)
            continue;
        if (static_cast<std::uint32_t>(addr) >= static_cast<std::uint32_t>(w.addr) + w.len)
            continue;
        w.last_value = val;
        w.last_writer_pc = pending_writer_pc_;
        w.has_fired = true;
        if (!pending_wp_hit_) {
            pending_wp_hit_ = true;
            pending_wp_addr_ = w.addr;
        }
    }
}

run_result debugger::step_over() {
    // CALL is $CD (unconditional) plus the cc variants $C4/$CC/$D4/$DC; RST
    // is the eight $C7..$FF / step-8 single-byte vector jumps. Anything else
    // (including HALT, JR, JP, RET) reduces to a plain step — the return-
    // address heuristic doesn't apply.
    const auto pc = core_.regs.pc;
    const auto op = core_.mmu.read_u8(pc);
    const bool is_call = op == 0xCD || op == 0xC4 || op == 0xCC || op == 0xD4 || op == 0xDC;
    const bool is_rst = (op & 0xC7) == 0xC7; // $C7,$CF,$D7,$DF,$E7,$EF,$F7,$FF

    if (!is_call && !is_rst) {
        const auto sr = step();
        run_result r{};
        r.cycles_consumed = sr.cycles;
        r.instructions = 1;
        if (sr.watchpoint_hit) {
            r.outcome = run_outcome::watchpoint;
            r.hit_addr = sr.watchpoint_addr;
        } else if (breakpoint_should_fire(core_.regs.pc)) {
            r.outcome = run_outcome::breakpoint;
            r.hit_addr = core_.regs.pc;
        } else {
            r.outcome = run_outcome::condition;
            r.hit_addr = core_.regs.pc;
        }
        return r;
    }

    const auto len = disasm_one(pc, core_.mmu).length;
    const auto target = static_cast<std::uint16_t>(pc + (len ? len : 1));

    // 1M T-cycles ≈ 4 GB frames: enough room for any realistic subroutine,
    // small enough that a never-returning callee doesn't lock the UI for
    // long.  If the cap fires we land mid-routine; the user can step from
    // there or set a breakpoint and Run.
    stop_condition cond{stop_kind::pc_eq, target, {}};
    return run_until(cond, 1'000'000);
}

void debugger::reset() {
    core_.reset();
}

run_result debugger::run_until(const stop_condition& cond, std::uint64_t max_cycles) {
    run_result r{};
    std::size_t serial_scan_pos = 0;

    while (r.cycles_consumed < max_cycles) {
        // Pre-step snapshot needed by `ld_b_b`: the opcode about to be fetched
        // by this iteration's step().  After step() the PC has moved past the
        // instruction (and a CALL/JP/IRQ may have rewritten it altogether), so
        // the opcode byte has to be sampled here.  We only pay the mmu read
        // for the kind that needs it — every other stop kind skips it.
        const auto pre_pc = core_.regs.pc;
        const std::uint8_t pre_op = (cond.kind == stop_kind::ld_b_b) ? core_.mmu.read_u8(pre_pc) : 0;

        const auto sr = step();
        r.cycles_consumed += sr.cycles;
        ++r.instructions;

        if (sr.watchpoint_hit) {
            r.outcome = run_outcome::watchpoint;
            r.hit_addr = sr.watchpoint_addr;
            return r;
        }
        if (breakpoint_should_fire(core_.regs.pc)) {
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
            case stop_kind::bg_text_match: {
                // Throttle the (1024 mmu reads + substring search) scan to one
                // pass per emulated frame — Blargg only repaints the result
                // text at VBlank, so per-step scanning would burn cycles for
                // no gain.  In headless mode no other consumer cares about
                // the frame_ready edge, so consuming it here is fine.
                if (cond.text_match.empty() || !core_.ppu.consume_frame_ready())
                    break;
                const auto s = bg_text_snapshot();
                if (s.find(cond.text_match) != std::string::npos) {
                    r.outcome = run_outcome::condition;
                    return r;
                }
                break;
            }
            case stop_kind::ld_b_b:
                if (pre_op == 0x40) {
                    r.outcome = run_outcome::condition;
                    r.hit_addr = pre_pc;
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
    // A plain `breakpoint_set(addr)` re-arms unconditional behaviour: any
    // previously-attached predicate at the same address is dropped.
    bp_predicates_.erase(
        std::remove_if(bp_predicates_.begin(), bp_predicates_.end(), [&](const auto& kv) { return kv.first == addr; }),
        bp_predicates_.end());
}

void debugger::breakpoint_set(std::uint16_t addr, bp_predicate pred) {
    bp_bitmap_[addr >> 3] |= static_cast<std::uint8_t>(1u << (addr & 7));
    if (pred.terms.empty()) {
        // Empty predicate -> unconditional; do not insert a redundant entry.
        bp_predicates_.erase(std::remove_if(bp_predicates_.begin(), bp_predicates_.end(),
                                            [&](const auto& kv) { return kv.first == addr; }),
                             bp_predicates_.end());
        return;
    }
    for (auto& kv : bp_predicates_) {
        if (kv.first == addr) {
            kv.second = std::move(pred);
            return;
        }
    }
    bp_predicates_.emplace_back(addr, std::move(pred));
}

void debugger::breakpoint_clear(std::uint16_t addr) {
    bp_bitmap_[addr >> 3] &= static_cast<std::uint8_t>(~(1u << (addr & 7)));
    bp_predicates_.erase(
        std::remove_if(bp_predicates_.begin(), bp_predicates_.end(), [&](const auto& kv) { return kv.first == addr; }),
        bp_predicates_.end());
}

void debugger::breakpoint_toggle(std::uint16_t addr) {
    const bool now_set = ((bp_bitmap_[addr >> 3] >> (addr & 7)) & 1) != 0;
    if (now_set) {
        breakpoint_clear(addr);
    } else {
        breakpoint_set(addr);
    }
}

bool debugger::breakpoint_has(std::uint16_t addr) const {
    return (bp_bitmap_[addr >> 3] & (1u << (addr & 7))) != 0;
}

bool debugger::breakpoint_should_fire(std::uint16_t addr) const {
    if (!breakpoint_has(addr))
        return false;
    for (const auto& kv : bp_predicates_) {
        if (kv.first == addr)
            return eval_bp_predicate(kv.second, core_);
    }
    return true; // unconditional breakpoint
}

const bp_predicate* debugger::breakpoint_predicate(std::uint16_t addr) const {
    for (const auto& kv : bp_predicates_) {
        if (kv.first == addr)
            return &kv.second;
    }
    return nullptr;
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
    watchpoints_.push_back(watchpoint{addr, len, 0, 0, false});
}

void debugger::watchpoint_clear(std::uint16_t addr) {
    watchpoints_.erase(
        std::remove_if(watchpoints_.begin(), watchpoints_.end(), [&](const watchpoint& w) { return w.addr == addr; }),
        watchpoints_.end());
}

void debugger::watchpoint_clear_all() {
    watchpoints_.clear();
}

const std::uint32_t* debugger::framebuffer() const {
    return core_.ppu.framebuffer();
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
    const auto rom_size = core_.mmu.read_u8(0x0148);
    const auto ram_size = core_.mmu.read_u8(0x0149);

    char buf[160];
    if (const auto* cart = core_.mmu.cart()) {
        const auto st = cart->debug_state();
        const auto cgb_flag = cart->cgb_flag();
        const char* cgb_label = "DMG";
        switch (classify_cgb_flag(cgb_flag)) {
            case cgb_support::compat:
                cgb_label = "CGB-compat";
                break;
            case cgb_support::cgb_only:
                cgb_label = "CGB-only";
                break;
            case cgb_support::none:
                break;
        }
        std::snprintf(buf, sizeof(buf), "cart_type=$%02X rom_size=$%02X ram_size=$%02X cgb_flag=$%02X(%s)\n", st.type,
                      rom_size, ram_size, cgb_flag, cgb_label);
        os << buf;
        std::snprintf(buf, sizeof(buf), "rom_bank=%u ram_bank=%u ram_enabled=%d mode=%u\n",
                      static_cast<unsigned>(st.rom_bank), static_cast<unsigned>(st.ram_bank), st.ram_enabled ? 1 : 0,
                      static_cast<unsigned>(st.mode));
        os << buf;
    } else {
        const auto cart_type = core_.mmu.read_u8(0x0147);
        std::snprintf(buf, sizeof(buf), "cart_type=$%02X rom_size=$%02X ram_size=$%02X\n", cart_type, rom_size,
                      ram_size);
        os << buf;
        os << "bank_state=(no cartridge attached)\n";
    }
}

std::string debugger::bg_text_snapshot() const {
    // Blargg's shell.inc text backend stores ASCII codes directly into the BG
    // tile map (the matching font glyph is loaded into VRAM at the tile index
    // == ASCII slot at boot), so reading the map back byte-for-byte yields
    // the on-screen text.  The full 32×32 map is scanned even though only
    // 20×18 tiles are visible: padding rows/columns contain Blargg's blank
    // tile ($7F or $00) which we collapse to space below.
    const auto lcdc = core_.mmu.hwr_lcdc();
    const std::uint16_t map_base = (lcdc & 0x08) ? 0x9C00 : 0x9800;
    std::string s;
    s.reserve(33 * 32);
    for (std::uint16_t row = 0; row < 32; ++row) {
        for (std::uint16_t col = 0; col < 32; ++col) {
            const auto t = core_.mmu.read_u8(static_cast<std::uint16_t>(map_base + row * 32 + col));
            s.push_back((t >= 0x20 && t < 0x7F) ? static_cast<char>(t) : ' ');
        }
        s.push_back('\n');
    }
    return s;
}

void debugger::dump_bg_text(std::ostream& os) const {
    os << bg_text_snapshot();
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
