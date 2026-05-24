#pragma once
#ifndef _H_DEBUGGER_H_
#    define _H_DEBUGGER_H_

#    include <array>
#    include <cstdint>
#    include <iosfwd>
#    include <string>
#    include <vector>

#    include <absl/container/inlined_vector.h>

namespace gbemu {
    struct core;

    // Watchpoint: range [addr, addr+len) sampled after every step.  When any
    // byte differs from the last sample, the watchpoint fires
    struct watchpoint {
        std::uint16_t addr;
        std::uint16_t len;
        absl::InlinedVector<std::uint8_t, 4> last;
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

        // Loop step() until the condition is satisfied, a breakpoint or
        // watchpoint fires, or `max_cycles` T-cycles have been consumed.
        // Default cap (200M cycles, ~50s emu time) keeps malformed scripts
        // from blocking the parent process.
        run_result run_until(const stop_condition& cond, std::uint64_t max_cycles = 200'000'000);

        std::uint64_t total_cycles() const;
        std::uint16_t current_pc() const;

        // Breakpoints — backed by an 8 KB bitmap (1 bit per 16-bit address),
        // so the per-step check is one load + one bit test.
        void breakpoint_set(std::uint16_t addr);
        void breakpoint_clear(std::uint16_t addr);
        void breakpoint_toggle(std::uint16_t addr);
        bool breakpoint_has(std::uint16_t addr) const;
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
        bool sample_watchpoints(std::uint16_t& out_addr);

        core& core_;
        std::vector<char> serial_buf_;
        std::array<std::uint8_t, 8192> bp_bitmap_{};
        absl::InlinedVector<watchpoint, 2> watchpoints_{};
        run_state state_{run_state::Running};
    };

} // namespace gbemu

#endif // _H_DEBUGGER_H_
