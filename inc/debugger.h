#pragma once
#ifndef _H_DEBUGGER_H_
#    define _H_DEBUGGER_H_

#    include <cstdint>
#    include <iosfwd>
#    include <vector>

namespace gbemu {
    struct core;

    // Debugger skeleton shared by the ImGui panels (PR3+) and the headless
    // script runner (PR2).  This PR1 version exposes only inspection-side
    // primitives — breakpoints, watchpoints, and run-until conditions land
    // in PR2.
    //
    // The debugger owns a serial ring buffer fed by an `add_mmio_write_handler`
    // it installs on $FF02 in its constructor; both the headless `serial-dump`
    // command and the PR5 ImGui serial panel read from there.
    struct debugger {
        explicit debugger(core& c);

        // Pass-through to core::step() for now.  PR2 wraps this with
        // breakpoint / watchpoint / run-until checks.
        std::uint32_t step();

        // Total T-cycles consumed since boot (delegates to core).
        std::uint64_t total_cycles() const;

        // Dump CPU register snapshot in the PR2 wire format:
        //   AF=XXXX BC=XXXX DE=XXXX HL=XXXX SP=XXXX PC=XXXX
        //   Z=X N=X H=X C=X IME=X HALT=X STOP=X
        void dump_regs(std::ostream& os) const;

        // Hex-dump `len` bytes starting at `addr`, 16 bytes per line.
        void dump_mem(std::ostream& os, std::uint16_t addr, std::uint16_t len) const;

        // Print the last `n` entries of the core's PC ring, newest first,
        // 8 per row.
        void dump_pc_ring(std::ostream& os, std::size_t n) const;

        // Disassemble `n` instructions starting at `addr`.
        void disasm(std::ostream& os, std::uint16_t addr, std::size_t n) const;

        // Serial ring buffer (bytes transmitted via SC=$81 while $FF02 was
        // written).  Read by the PR2 `serial-dump` command and the PR5 ImGui
        // serial panel.  Cleared by serial_clear().
        const std::vector<char>& serial_buffer() const { return serial_buf_; }
        void serial_clear() { serial_buf_.clear(); }

    private:
        core& core_;
        // TODO(small_vector): bounded ring would be more honest than an
        // unbounded vector; small_vector won't help here (could be many KB).
        std::vector<char> serial_buf_;
    };

} // namespace gbemu

#endif // _H_DEBUGGER_H_
