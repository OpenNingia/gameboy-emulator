#pragma once
#ifndef _H_BP_PREDICATE_H_
#    define _H_BP_PREDICATE_H_

#    include <cstdint>
#    include <optional>
#    include <string>
#    include <string_view>
#    include <vector>

namespace gbemu {
    struct core;

    // Conditional breakpoint predicate.  Grammar (v1, no precedence, no
    // parentheses other than memory-deref `(...)`, no boolean OR):
    //
    //   predicate := term ( "&&" term )*
    //   term      := lhs op rhs
    //   lhs       := reg | "(" reg16_or_addr ")"
    //   reg       := A|F|B|C|D|E|H|L|AF|BC|DE|HL|SP|PC   (case-insensitive)
    //   reg16     := BC|DE|HL|SP
    //   op        := "==" | "=" | "!=" | "<=" | ">=" | "<" | ">"
    //   rhs       := decimal | "0x" hex | "$" hex
    //
    // 8-bit lhs (`A`, `F`, mem dereferences) compare against the low byte
    // of rhs; 16-bit lhs compare against the full 16-bit rhs.
    struct bp_term {
        enum class lhs_kind { reg, mem_reg, mem_abs };
        enum class reg_id { A, F, B, C, D, E, H, L, AF, BC, DE, HL, SP, PC };
        enum class op_kind { eq, ne, lt, le, gt, ge };

        lhs_kind lhs{lhs_kind::reg};
        reg_id reg{reg_id::A}; // when lhs == reg or mem_reg (BC/DE/HL/SP)
        std::uint16_t addr{0}; // when lhs == mem_abs
        op_kind op{op_kind::eq};
        std::uint16_t rhs{0};
    };

    struct bp_predicate {
        std::vector<bp_term> terms; // ANDed
        std::string source;         // original (trimmed) text, for display
    };

    // Parse a predicate string.  Returns nullopt on failure and writes a
    // human-readable explanation to `err`.  An empty / whitespace-only input
    // parses to an "always true" predicate (empty `terms`).
    std::optional<bp_predicate> parse_bp_predicate(std::string_view s, std::string& err);

    // Evaluate against the live core state.  An empty `terms` list is
    // unconditionally true (this matches "no condition attached").
    bool eval_bp_predicate(const bp_predicate& p, const core& c);

} // namespace gbemu

#endif // _H_BP_PREDICATE_H_
