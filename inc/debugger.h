#pragma once
#ifndef _H_DEBUGGER_H_
#    define _H_DEBUGGER_H_

#    include <array>
#    include <cstdint>
#    include <iosfwd>
#    include <string>
#    include <vector>

#    include <absl/container/inlined_vector.h>
#    include <bp_predicate.h>

namespace gbemu {
    struct core;

    // Watchpoint: range [addr, addr+len) sampled after every step.  When any
    // byte differs from the last sample, the watchpoint fires.
    //
    // `last_writer_pc` captures the PC of the instruction that was about to
    // execute when the change was observed (i.e. the writer).  It is the PC
    // *before* the step that triggered the change, so a CALL/JP/IRQ that
    // moved PC after the write still resolves to the originating instruction.
    // Valid only when `has_fired` is true (a freshly seeded watchpoint has
    // no writer yet).
    struct watchpoint {
        std::uint16_t addr;
        std::uint16_t len;
        absl::InlinedVector<std::uint8_t, 4> last;
        std::uint16_t last_writer_pc{0};
        bool has_fired{false};
    };

    enum class stop_kind {
        none,         // run until breakpoint / max_cycles (no user condition)
        pc_eq,        // stop when PC reaches `value`
        cycles_ge,    // stop when core.total_cycles >= value
        serial_match, // stop when serial_buffer contains `serial_match`
        vblank,       // stop on the next PPU frame-ready edge
        instr_count,  // stop after `value` instructions executed in this run
    };

    struct stop_condition {
        stop_kind kind{stop_kind::none};
        std::uint64_t value{0};
        std::string serial_match{};
    };

    struct step_result {
        std::uint32_t cycles{0};
        bool watchpoint_hit{false};
        std::uint16_t watchpoint_addr{0};
        // PC of the instruction whose execution triggered the watchpoint
        // (the pre-step PC).  Meaningful only when `watchpoint_hit` is true.
        std::uint16_t watchpoint_writer_pc{0};
    };

    enum class run_outcome {
        condition,         // user-supplied stop_condition was satisfied
        breakpoint,        // a PC breakpoint fired (always interrupts)
        watchpoint,        // a watchpoint fired
        max_cycles_safety, // hit the safety cap without satisfying the condition
    };

    // Coarse-grain execution mode driven by the ImGui CPU panel (Run/Pause
    // buttons).  Headless / script-runner usage doesn't consult this — it
    // only gates the SDL main loop's CPU step in interactive mode.
    enum class run_state {
        Running,
        Paused,
    };

    struct run_result {
        run_outcome outcome{run_outcome::max_cycles_safety};
        std::uint16_t hit_addr{0};
        std::uint64_t cycles_consumed{0};
        std::uint64_t instructions{0};
    };

    // Debugger skeleton shared by the ImGui panels and the headless script runner
    //
    // The debugger owns a serial ring buffer fed by a handler it installs on
    // $FF02 in its constructor; both the headless `serial-dump` command and
    // the PR5 ImGui serial panel read from there.
    struct debugger {
        explicit debugger(core& c);

        // Execute one instruction.  Always advances (no breakpoint check),
        // so a `step` from inside a breakpoint moves past it.  Watchpoint
        // bytes are sampled and compared after the step.
        step_result step();

        // Step Over: when PC is on a CALL or RST, runs until PC reaches the
        // instruction after it (i.e. the return address) and returns the
        // run_result.  On any other instruction it behaves like step().  The
        // pc_eq cap is intentionally small (~1M T-cycles, ~4 frames) so a
        // never-returning subroutine doesn't lock the UI; in that case the
        // user lands wherever the cap fired, same as if they'd been Running.
        run_result step_over();

        // Hard reset of the emulated machine: zeroes registers, RAM, VRAM,
        // OAM, MMIO and HRAM; resets every subsystem's internal state;
        // preserves the loaded BIOS image (so BIOS re-runs if it was active)
        // and the attached cartridge (banking state is reset, ROM bytes are
        // not).  Breakpoints, watchpoints and the serial scrollback are
        // debugger-side state and persist across reset.
        void reset();

        // Loop step() until the condition is satisfied, a breakpoint or
        // watchpoint fires, or `max_cycles` T-cycles have been consumed.
        // Default cap (200M cycles, ~50s emu time) keeps malformed scripts
        // from blocking the parent process.
        run_result run_until(const stop_condition& cond, std::uint64_t max_cycles = 200'000'000);

        std::uint64_t total_cycles() const;
        std::uint16_t current_pc() const;

        // Breakpoints — backed by an 8 KB bitmap (1 bit per 16-bit address),
        // so the per-step check is one load + one bit test.  Conditional
        // breakpoints attach a parsed `bp_predicate` to the address via a
        // small side table; `breakpoint_should_fire` evaluates it when the
        // bitmap matches.  The bitmap is also the truth for the disassembly
        // panel's red-square marker (`breakpoint_has`) — both conditional
        // and unconditional breakpoints display identically there.
        void breakpoint_set(std::uint16_t addr);
        void breakpoint_set(std::uint16_t addr, bp_predicate pred);
        void breakpoint_clear(std::uint16_t addr);
        void breakpoint_toggle(std::uint16_t addr);
        bool breakpoint_has(std::uint16_t addr) const;
        // True iff a breakpoint at `addr` should pause execution now.  When
        // the address carries a predicate, the predicate is evaluated
        // against the current core state.  A bitmap-only breakpoint always
        // fires; an unknown address never fires.
        bool breakpoint_should_fire(std::uint16_t addr) const;
        // Returns the predicate attached to `addr`, or nullptr when the
        // breakpoint is unconditional or absent.  The pointer is borrowed
        // and invalidated by the next breakpoint mutation.
        const bp_predicate* breakpoint_predicate(std::uint16_t addr) const;
        std::vector<std::uint16_t> breakpoint_list() const;

        // Watchpoints.
        void watchpoint_set(std::uint16_t addr, std::uint16_t len);
        void watchpoint_clear(std::uint16_t addr);
        void watchpoint_clear_all();
        const absl::InlinedVector<watchpoint, 2>& watchpoints() const { return watchpoints_; }

        // Inspection (write to caller-supplied ostream; the script runner
        // wraps these with `=== ... ===` headers).
        void dump_regs(std::ostream& os) const;
        void dump_mem(std::ostream& os, std::uint16_t addr, std::uint16_t len) const;
        void dump_pc_ring(std::ostream& os, std::size_t n) const;
        void disasm(std::ostream& os, std::uint16_t addr, std::size_t n) const;
        void dump_ppu(std::ostream& os) const;
        void dump_mbc(std::ostream& os) const;
        void dump_stack(std::ostream& os, std::size_t n) const;

        // Serial buffer (populated by handler installed in ctor on $FF02).
        const std::vector<char>& serial_buffer() const { return serial_buf_; }
        void serial_clear() { serial_buf_.clear(); }

        // Run-state controls (interactive UI only).
        bool is_paused() const { return state_ == run_state::Paused; }
        void pause() { state_ = run_state::Paused; }
        void resume() { state_ = run_state::Running; }
        void toggle_running() { state_ = is_paused() ? run_state::Running : run_state::Paused; }

    private:
        // Returns true and refreshes `last` if any watched byte changed.  Caller
        // gets the first changed watchpoint's base address via `out_addr`.
        // `writer_pc` is recorded into every changed watchpoint's
        // `last_writer_pc` field — it is the PC of the instruction that just
        // executed, captured by the caller before the step.
        bool sample_watchpoints(std::uint16_t& out_addr, std::uint16_t writer_pc);

        core& core_;
        std::vector<char> serial_buf_;
        std::array<std::uint8_t, 8192> bp_bitmap_{};
        // Predicates attached to conditional breakpoints.  Linear-scanned —
        // expected count is small (handful of bps at most), and entries are
        // only consulted when the bitmap fast-path already matched the PC.
        std::vector<std::pair<std::uint16_t, bp_predicate>> bp_predicates_{};
        absl::InlinedVector<watchpoint, 2> watchpoints_{};
        run_state state_{run_state::Running};
    };

} // namespace gbemu

#endif // _H_DEBUGGER_H_
