#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

#include <bp_predicate.h>
#include <core.h>

namespace gbemu {

    namespace {

        bool is_space(char c) {
            return std::isspace(static_cast<unsigned char>(c)) != 0;
        }

        void trim(std::string_view& s) {
            while (!s.empty() && is_space(s.front()))
                s.remove_prefix(1);
            while (!s.empty() && is_space(s.back()))
                s.remove_suffix(1);
        }

        std::string to_upper(std::string_view s) {
            std::string out(s);
            for (auto& c : out)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return out;
        }

        // Returns nullopt when `s` is not a recognised register name.
        std::optional<bp_term::reg_id> match_reg(std::string_view s) {
            const auto u = to_upper(s);
            using r = bp_term::reg_id;
            if (u == "A")
                return r::A;
            if (u == "F")
                return r::F;
            if (u == "B")
                return r::B;
            if (u == "C")
                return r::C;
            if (u == "D")
                return r::D;
            if (u == "E")
                return r::E;
            if (u == "H")
                return r::H;
            if (u == "L")
                return r::L;
            if (u == "AF")
                return r::AF;
            if (u == "BC")
                return r::BC;
            if (u == "DE")
                return r::DE;
            if (u == "HL")
                return r::HL;
            if (u == "SP")
                return r::SP;
            if (u == "PC")
                return r::PC;
            return std::nullopt;
        }

        bool is_reg16(bp_term::reg_id r) {
            using R = bp_term::reg_id;
            return r == R::AF || r == R::BC || r == R::DE || r == R::HL || r == R::SP || r == R::PC;
        }

        // Accept decimal / 0xNN / $NN.  Throws on parse failure (caller wraps).
        std::uint32_t parse_number(std::string_view s) {
            std::string t(s);
            if (t.empty())
                throw std::runtime_error("empty number");
            std::size_t pos = 0;
            unsigned long val = 0;
            if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
                val = std::stoul(t.substr(2), &pos, 16), pos += 2;
            else if (t.size() > 1 && t[0] == '$')
                val = std::stoul(t.substr(1), &pos, 16), pos += 1;
            else
                val = std::stoul(t, &pos, 10);
            if (pos != t.size())
                throw std::runtime_error("trailing garbage in number '" + t + "'");
            return static_cast<std::uint32_t>(val);
        }

        // Returns (op, op_len_in_chars).  Multi-char operators must be tested
        // first so `<=` doesn't mis-tokenise as `<` + `=`.
        std::optional<std::pair<bp_term::op_kind, std::size_t>> match_op(std::string_view s) {
            using O = bp_term::op_kind;
            if (s.size() >= 2) {
                if (s.substr(0, 2) == "==")
                    return std::make_pair(O::eq, std::size_t{2});
                if (s.substr(0, 2) == "!=")
                    return std::make_pair(O::ne, std::size_t{2});
                if (s.substr(0, 2) == "<=")
                    return std::make_pair(O::le, std::size_t{2});
                if (s.substr(0, 2) == ">=")
                    return std::make_pair(O::ge, std::size_t{2});
            }
            if (!s.empty()) {
                if (s[0] == '=')
                    return std::make_pair(O::eq, std::size_t{1});
                if (s[0] == '<')
                    return std::make_pair(O::lt, std::size_t{1});
                if (s[0] == '>')
                    return std::make_pair(O::gt, std::size_t{1});
            }
            return std::nullopt;
        }

        // Find the next operator in `s` and return its (offset, op, length).
        // Memory dereferences `(...)` shield any inner characters from match,
        // but at this grammar level there is no nested deref anyway.
        std::optional<std::tuple<std::size_t, bp_term::op_kind, std::size_t>> find_op(std::string_view s) {
            std::size_t depth = 0;
            for (std::size_t i = 0; i < s.size(); ++i) {
                if (s[i] == '(') {
                    ++depth;
                    continue;
                }
                if (s[i] == ')') {
                    if (depth)
                        --depth;
                    continue;
                }
                if (depth)
                    continue;
                if (auto op = match_op(s.substr(i)))
                    return std::make_tuple(i, op->first, op->second);
            }
            return std::nullopt;
        }

        bool parse_term(std::string_view raw, bp_term& out, std::string& err) {
            trim(raw);
            if (raw.empty()) {
                err = "empty term";
                return false;
            }
            const auto op_pos = find_op(raw);
            if (!op_pos) {
                err = "missing comparison operator in term '" + std::string(raw) + "'";
                return false;
            }
            const auto [pos, op, op_len] = *op_pos;
            std::string_view lhs_tok = raw.substr(0, pos);
            std::string_view rhs_tok = raw.substr(pos + op_len);
            trim(lhs_tok);
            trim(rhs_tok);
            if (lhs_tok.empty()) {
                err = "missing left-hand side";
                return false;
            }
            if (rhs_tok.empty()) {
                err = "missing right-hand side";
                return false;
            }

            out.op = op;

            // Memory deref form (REG16) or (NUMBER).
            if (lhs_tok.front() == '(') {
                if (lhs_tok.back() != ')') {
                    err = "unbalanced '(' in '" + std::string(lhs_tok) + "'";
                    return false;
                }
                std::string_view inner = lhs_tok;
                inner.remove_prefix(1);
                inner.remove_suffix(1);
                trim(inner);
                if (inner.empty()) {
                    err = "empty memory deref";
                    return false;
                }
                if (auto r = match_reg(inner)) {
                    using R = bp_term::reg_id;
                    if (*r != R::BC && *r != R::DE && *r != R::HL && *r != R::SP) {
                        err = "memory deref accepts BC/DE/HL/SP only";
                        return false;
                    }
                    out.lhs = bp_term::lhs_kind::mem_reg;
                    out.reg = *r;
                } else {
                    try {
                        out.lhs = bp_term::lhs_kind::mem_abs;
                        out.addr = static_cast<std::uint16_t>(parse_number(inner));
                    } catch (const std::exception& e) {
                        err = std::string("bad memory deref operand: ") + e.what();
                        return false;
                    }
                }
            } else {
                auto r = match_reg(lhs_tok);
                if (!r) {
                    err = "unknown register '" + std::string(lhs_tok) + "'";
                    return false;
                }
                out.lhs = bp_term::lhs_kind::reg;
                out.reg = *r;
            }

            try {
                out.rhs = static_cast<std::uint16_t>(parse_number(rhs_tok));
            } catch (const std::exception& e) {
                err = std::string("bad rhs: ") + e.what();
                return false;
            }
            return true;
        }

        std::uint16_t read_reg(const core& c, bp_term::reg_id r) {
            using R = bp_term::reg_id;
            switch (r) {
                case R::A:
                    return c.regs.af.hi;
                case R::F:
                    return c.regs.af.lo;
                case R::B:
                    return c.regs.bc.hi;
                case R::C:
                    return c.regs.bc.lo;
                case R::D:
                    return c.regs.de.hi;
                case R::E:
                    return c.regs.de.lo;
                case R::H:
                    return c.regs.hl.hi;
                case R::L:
                    return c.regs.hl.lo;
                case R::AF:
                    return c.regs.af.u16;
                case R::BC:
                    return c.regs.bc.u16;
                case R::DE:
                    return c.regs.de.u16;
                case R::HL:
                    return c.regs.hl.u16;
                case R::SP:
                    return c.regs.sp;
                case R::PC:
                    return c.regs.pc;
            }
            return 0;
        }

        bool is_8bit_lhs(const bp_term& t) {
            using K = bp_term::lhs_kind;
            using R = bp_term::reg_id;
            if (t.lhs == K::mem_reg || t.lhs == K::mem_abs)
                return true;
            switch (t.reg) {
                case R::A:
                case R::F:
                case R::B:
                case R::C:
                case R::D:
                case R::E:
                case R::H:
                case R::L:
                    return true;
                default:
                    return false;
            }
        }

        bool apply_op(bp_term::op_kind op, std::uint16_t lhs, std::uint16_t rhs) {
            using O = bp_term::op_kind;
            switch (op) {
                case O::eq:
                    return lhs == rhs;
                case O::ne:
                    return lhs != rhs;
                case O::lt:
                    return lhs < rhs;
                case O::le:
                    return lhs <= rhs;
                case O::gt:
                    return lhs > rhs;
                case O::ge:
                    return lhs >= rhs;
            }
            return false;
        }

    } // namespace

    std::optional<bp_predicate> parse_bp_predicate(std::string_view s, std::string& err) {
        err.clear();
        bp_predicate p;
        trim(s);
        p.source.assign(s.begin(), s.end());
        if (s.empty())
            return p; // empty predicate -> always true (no condition).

        std::size_t i = 0;
        std::size_t term_start = 0;
        while (i < s.size()) {
            if (i + 1 < s.size() && s[i] == '&' && s[i + 1] == '&') {
                bp_term t{};
                if (!parse_term(s.substr(term_start, i - term_start), t, err))
                    return std::nullopt;
                p.terms.push_back(t);
                i += 2;
                term_start = i;
            } else {
                ++i;
            }
        }
        bp_term t{};
        if (!parse_term(s.substr(term_start), t, err))
            return std::nullopt;
        p.terms.push_back(t);
        return p;
    }

    bool eval_bp_predicate(const bp_predicate& p, const core& c) {
        if (p.terms.empty())
            return true;
        for (const auto& t : p.terms) {
            std::uint16_t lhs = 0;
            switch (t.lhs) {
                case bp_term::lhs_kind::reg:
                    lhs = read_reg(c, t.reg);
                    break;
                case bp_term::lhs_kind::mem_reg: {
                    const auto addr = read_reg(c, t.reg);
                    lhs = c.mmu.read_u8(addr);
                    break;
                }
                case bp_term::lhs_kind::mem_abs:
                    lhs = c.mmu.read_u8(t.addr);
                    break;
            }
            // 8-bit operands mask the rhs to a byte so `A == 0x1FF` does not
            // silently never fire (A is one byte; rhs is documented as
            // truncated to lhs width).
            std::uint16_t rhs = t.rhs;
            if (is_8bit_lhs(t)) {
                lhs &= 0xFF;
                rhs &= 0xFF;
            }
            if (!apply_op(t.op, lhs, rhs))
                return false;
        }
        return true;
    }

} // namespace gbemu
