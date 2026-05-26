// Headless debug-script runner.  Each line in the script is a command
// dispatched against a `debugger&`; output is written as key=value blocks
// separated by `=== ... ===` headers (and `--- text ---` echoes) so a
// downstream consumer (Claude reading the dump file, or grep) can locate
// sections deterministically.
//
// Syntax (whitespace-separated tokens, double quotes for strings):
//   step N
//   step-cycles N
//   run-until pc ADDR | cycles N | serial-match "TEXT" | vblank | bp |
//              instr-count N    [max-cycles N]
//   break ADDR [COND...]  /  break-clear ADDR  /  break-clear-all  /  break-list
//   watch ADDR [LEN]  /  watch-clear ADDR  /  watch-clear-all  /  watch-list
//   dump regs | mem ADDR LEN | ppu | mbc | pc-ring [N] | stack [N]
//   disasm pc [N]  /  disasm ADDR [N]
//   serial-clear  /  serial-dump
//   echo TEXT...
//   exit
// Numeric arguments accept decimal, `0xNN` or `$NN` for hex.
// Lines starting with `#` are comments; blank lines are skipped.

#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

#include <debugger.h>

namespace gbemu {

    namespace {

        class tokenizer {
        public:
            explicit tokenizer(const std::string& s) : s_(s), pos_(0) {}

            bool empty() {
                skip_ws();
                return pos_ >= s_.size();
            }

            std::string next() {
                skip_ws();
                if (pos_ >= s_.size())
                    return {};
                if (s_[pos_] == '"') {
                    ++pos_;
                    const std::size_t start = pos_;
                    while (pos_ < s_.size() && s_[pos_] != '"')
                        ++pos_;
                    std::string t = s_.substr(start, pos_ - start);
                    if (pos_ < s_.size())
                        ++pos_;
                    return t;
                }
                const std::size_t start = pos_;
                while (pos_ < s_.size() && !std::isspace(static_cast<unsigned char>(s_[pos_])))
                    ++pos_;
                return s_.substr(start, pos_ - start);
            }

            // Returns everything remaining on the line, preserving internal
            // whitespace.  Used by `echo`.
            std::string rest() {
                skip_ws();
                std::string r = s_.substr(pos_);
                pos_ = s_.size();
                // strip trailing whitespace
                while (!r.empty() && std::isspace(static_cast<unsigned char>(r.back())))
                    r.pop_back();
                return r;
            }

        private:
            void skip_ws() {
                while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_])))
                    ++pos_;
            }

            const std::string& s_;
            std::size_t pos_;
        };

        std::uint64_t parse_u64(const std::string& s) {
            if (s.empty())
                throw std::invalid_argument("expected number");
            if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
                return std::stoull(s.substr(2), nullptr, 16);
            if (s.size() > 1 && s[0] == '$')
                return std::stoull(s.substr(1), nullptr, 16);
            return std::stoull(s, nullptr, 10);
        }

        std::uint16_t parse_addr(const std::string& s) {
            return static_cast<std::uint16_t>(parse_u64(s));
        }

        std::string hex_addr(std::uint16_t v) {
            char b[8];
            std::snprintf(b, sizeof(b), "$%04X", v);
            return b;
        }

        const char* outcome_name(run_outcome o) {
            switch (o) {
                case run_outcome::condition:
                    return "condition";
                case run_outcome::breakpoint:
                    return "breakpoint";
                case run_outcome::watchpoint:
                    return "watchpoint";
                case run_outcome::max_cycles_safety:
                    return "max_cycles_safety";
            }
            return "?";
        }

        void cmd_step(debugger& dbg, tokenizer& ts, std::ostream& out) {
            const auto n = parse_u64(ts.next());
            const auto cycle_before = dbg.total_cycles();
            std::uint64_t executed = 0;
            bool stopped = false;
            std::uint16_t hit = 0;
            for (std::uint64_t i = 0; i < n; ++i) {
                const auto r = dbg.step();
                ++executed;
                if (r.watchpoint_hit) {
                    stopped = true;
                    hit = r.watchpoint_addr;
                    break;
                }
            }
            out << "=== step " << n << " @cycle=" << dbg.total_cycles() << " ===\n";
            out << "executed=" << executed << " cycles=" << (dbg.total_cycles() - cycle_before);
            if (stopped)
                out << " stopped=watchpoint hit=" << hex_addr(hit);
            out << "\n";
        }

        void cmd_step_cycles(debugger& dbg, tokenizer& ts, std::ostream& out) {
            const auto target = parse_u64(ts.next());
            const auto start = dbg.total_cycles();
            std::uint64_t executed = 0;
            bool stopped = false;
            std::uint16_t hit = 0;
            while (dbg.total_cycles() - start < target) {
                const auto r = dbg.step();
                ++executed;
                if (r.watchpoint_hit) {
                    stopped = true;
                    hit = r.watchpoint_addr;
                    break;
                }
            }
            out << "=== step-cycles " << target << " @cycle=" << dbg.total_cycles() << " ===\n";
            out << "instructions=" << executed << " cycles=" << (dbg.total_cycles() - start);
            if (stopped)
                out << " stopped=watchpoint hit=" << hex_addr(hit);
            out << "\n";
        }

        void cmd_run_until(debugger& dbg, tokenizer& ts, std::ostream& out) {
            const auto kind_tok = ts.next();
            stop_condition cond{};
            if (kind_tok == "pc") {
                cond.kind = stop_kind::pc_eq;
                cond.value = parse_u64(ts.next());
            } else if (kind_tok == "cycles") {
                cond.kind = stop_kind::cycles_ge;
                cond.value = parse_u64(ts.next());
            } else if (kind_tok == "serial-match") {
                cond.kind = stop_kind::serial_match;
                cond.serial_match = ts.next();
            } else if (kind_tok == "vblank") {
                cond.kind = stop_kind::vblank;
            } else if (kind_tok == "bp") {
                cond.kind = stop_kind::none;
            } else if (kind_tok == "instr-count") {
                cond.kind = stop_kind::instr_count;
                cond.value = parse_u64(ts.next());
            } else {
                out << "ERROR: run-until: unknown kind '" << kind_tok << "'\n";
                return;
            }

            std::uint64_t max_cycles = 200'000'000;
            while (!ts.empty()) {
                const auto k = ts.next();
                if (k == "max-cycles")
                    max_cycles = parse_u64(ts.next());
                else {
                    out << "ERROR: run-until: unknown modifier '" << k << "'\n";
                    return;
                }
            }

            const auto rr = dbg.run_until(cond, max_cycles);
            out << "=== run-until " << kind_tok << " @cycle=" << dbg.total_cycles() << " ===\n";
            out << "outcome=" << outcome_name(rr.outcome);
            if (rr.outcome == run_outcome::breakpoint || rr.outcome == run_outcome::watchpoint ||
                rr.outcome == run_outcome::condition) {
                if (cond.kind == stop_kind::pc_eq || rr.outcome == run_outcome::breakpoint ||
                    rr.outcome == run_outcome::watchpoint) {
                    out << " hit=" << hex_addr(rr.hit_addr);
                }
            }
            out << " cycles_consumed=" << rr.cycles_consumed << " instructions=" << rr.instructions << "\n";
        }

        void cmd_break(debugger& dbg, tokenizer& ts, std::ostream& out) {
            const auto addr = parse_addr(ts.next());
            // Anything left on the line is treated as the predicate text.
            // Empty -> unconditional breakpoint (matches the old behaviour).
            const auto cond = ts.rest();
            if (cond.empty()) {
                dbg.breakpoint_set(addr);
                return;
            }
            std::string err;
            if (auto p = parse_bp_predicate(cond, err)) {
                if (p->terms.empty())
                    dbg.breakpoint_set(addr);
                else
                    dbg.breakpoint_set(addr, std::move(*p));
            } else {
                out << "ERROR: break: " << err << "\n";
            }
        }

        void cmd_break_clear(debugger& dbg, tokenizer& ts, std::ostream&) {
            dbg.breakpoint_clear(parse_addr(ts.next()));
        }

        void cmd_break_list(debugger& dbg, tokenizer&, std::ostream& out) {
            const auto bps = dbg.breakpoint_list();
            out << "=== breakpoints " << bps.size() << " ===\n";
            for (auto a : bps) {
                out << hex_addr(a);
                if (const auto* pred = dbg.breakpoint_predicate(a))
                    out << " if " << pred->source;
                out << "\n";
            }
        }

        void cmd_watch(debugger& dbg, tokenizer& ts, std::ostream&) {
            const auto addr = parse_addr(ts.next());
            std::uint16_t len = 1;
            if (!ts.empty())
                len = static_cast<std::uint16_t>(parse_u64(ts.next()));
            dbg.watchpoint_set(addr, len);
        }

        void cmd_watch_clear(debugger& dbg, tokenizer& ts, std::ostream&) {
            dbg.watchpoint_clear(parse_addr(ts.next()));
        }

        void cmd_watch_list(debugger& dbg, tokenizer&, std::ostream& out) {
            const auto& ws = dbg.watchpoints();
            out << "=== watchpoints " << ws.size() << " ===\n";
            for (const auto& w : ws) {
                out << hex_addr(w.addr) << " len=" << w.len;
                if (w.has_fired)
                    out << " writer=" << hex_addr(w.last_writer_pc);
                out << "\n";
            }
        }

        void cmd_dump(debugger& dbg, tokenizer& ts, std::ostream& out) {
            const auto kind = ts.next();
            if (kind == "regs") {
                out << "=== regs @cycle=" << dbg.total_cycles() << " ===\n";
                dbg.dump_regs(out);
            } else if (kind == "mem") {
                const auto addr = parse_addr(ts.next());
                const auto len = static_cast<std::uint16_t>(parse_u64(ts.next()));
                out << "=== mem " << hex_addr(addr) << " " << len << " ===\n";
                dbg.dump_mem(out, addr, len);
            } else if (kind == "ppu") {
                out << "=== ppu @cycle=" << dbg.total_cycles() << " ===\n";
                dbg.dump_ppu(out);
            } else if (kind == "mbc") {
                out << "=== mbc ===\n";
                dbg.dump_mbc(out);
            } else if (kind == "pc-ring") {
                std::size_t n = 16;
                if (!ts.empty())
                    n = static_cast<std::size_t>(parse_u64(ts.next()));
                out << "=== pc-ring " << n << " ===\n";
                dbg.dump_pc_ring(out, n);
            } else if (kind == "stack") {
                std::size_t n = 8;
                if (!ts.empty())
                    n = static_cast<std::size_t>(parse_u64(ts.next()));
                out << "=== stack " << n << " ===\n";
                dbg.dump_stack(out, n);
            } else {
                out << "ERROR: dump: unknown kind '" << kind << "'\n";
            }
        }

        void cmd_disasm(debugger& dbg, tokenizer& ts, std::ostream& out) {
            const auto first = ts.next();
            std::uint16_t addr;
            if (first == "pc") {
                addr = dbg.current_pc();
            } else {
                addr = parse_addr(first);
            }
            std::size_t n = 4;
            if (!ts.empty())
                n = static_cast<std::size_t>(parse_u64(ts.next()));
            out << "=== disasm " << hex_addr(addr) << " " << n << " ===\n";
            dbg.disasm(out, addr, n);
        }

        void cmd_serial_dump(debugger& dbg, tokenizer&, std::ostream& out) {
            const auto& buf = dbg.serial_buffer();
            out << "=== serial @cycle=" << dbg.total_cycles() << " len=" << buf.size() << " ===\n";
            out.write(buf.data(), static_cast<std::streamsize>(buf.size()));
            if (buf.empty() || buf.back() != '\n')
                out << "\n";
        }

    } // namespace

    // Returns 0 on a clean exit (script reached `exit` or EOF), non-zero on
    // an I/O failure opening the script or output file.
    int run_script(debugger& dbg, const std::string& script_path, const std::string& out_path) {
        std::ifstream script(script_path);
        if (!script) {
            std::cerr << "Cannot open script: " << script_path << "\n";
            return 1;
        }
        std::ofstream out(out_path);
        if (!out) {
            std::cerr << "Cannot open output: " << out_path << "\n";
            return 1;
        }

        std::string line;
        int linenum = 0;
        while (std::getline(script, line)) {
            ++linenum;
            // strip trailing CR (CRLF scripts)
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            // skip leading whitespace
            std::size_t lead = 0;
            while (lead < line.size() && std::isspace(static_cast<unsigned char>(line[lead])))
                ++lead;
            if (lead == line.size() || line[lead] == '#')
                continue;

            tokenizer ts(line);
            const auto cmd = ts.next();

            try {
                if (cmd == "step")
                    cmd_step(dbg, ts, out);
                else if (cmd == "step-cycles")
                    cmd_step_cycles(dbg, ts, out);
                else if (cmd == "run-until")
                    cmd_run_until(dbg, ts, out);
                else if (cmd == "break")
                    cmd_break(dbg, ts, out);
                else if (cmd == "break-clear")
                    cmd_break_clear(dbg, ts, out);
                else if (cmd == "break-list")
                    cmd_break_list(dbg, ts, out);
                else if (cmd == "watch")
                    cmd_watch(dbg, ts, out);
                else if (cmd == "watch-clear")
                    cmd_watch_clear(dbg, ts, out);
                else if (cmd == "watch-clear-all")
                    dbg.watchpoint_clear_all();
                else if (cmd == "watch-list")
                    cmd_watch_list(dbg, ts, out);
                else if (cmd == "dump")
                    cmd_dump(dbg, ts, out);
                else if (cmd == "disasm")
                    cmd_disasm(dbg, ts, out);
                else if (cmd == "serial-clear")
                    dbg.serial_clear();
                else if (cmd == "serial-dump")
                    cmd_serial_dump(dbg, ts, out);
                else if (cmd == "echo")
                    out << "--- " << ts.rest() << " ---\n";
                else if (cmd == "exit")
                    break;
                else
                    out << "ERROR: unknown command '" << cmd << "' (line " << linenum << ")\n";
            } catch (const std::exception& e) {
                out << "ERROR: " << cmd << " (line " << linenum << "): " << e.what() << "\n";
            }

            out.flush();
        }

        return 0;
    }

} // namespace gbemu
