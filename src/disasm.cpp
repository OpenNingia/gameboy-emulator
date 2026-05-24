#include <cstdio>
#include <string>
#include <string_view>

#include <disasm.h>
#include <disasm_table.hpp>
#include <mmu.h>

using namespace gbemu;

namespace {

    void substitute_once(std::string& tpl, std::string_view needle, std::string_view replacement) {
        const auto pos = tpl.find(needle);
        if (pos != std::string::npos)
            tpl.replace(pos, needle.size(), replacement);
    }

} // namespace

disasm_result gbemu::disasm_one(std::uint16_t addr, const mmu& m) {
    disasm_result r{};
    r.bytes[0] = m.read_u8(addr);
    r.bytes[1] = m.read_u8(static_cast<std::uint16_t>(addr + 1));
    r.bytes[2] = m.read_u8(static_cast<std::uint16_t>(addr + 2));

    const disasm_entry* e = (r.bytes[0] == 0xCB) ? &disasm_table_cb[r.bytes[1]] : &disasm_table[r.bytes[0]];
    r.length = e->length;

    std::string tpl{e->mnemonic};
    char buf[16];

    // Longer placeholders first so d16/a16 don't get half-eaten by d8/a8.
    if (tpl.find("d16") != std::string::npos) {
        const std::uint16_t w = static_cast<std::uint16_t>(r.bytes[1] | (r.bytes[2] << 8));
        std::snprintf(buf, sizeof(buf), "$%04X", w);
        substitute_once(tpl, "d16", buf);
    }
    if (tpl.find("a16") != std::string::npos) {
        const std::uint16_t w = static_cast<std::uint16_t>(r.bytes[1] | (r.bytes[2] << 8));
        std::snprintf(buf, sizeof(buf), "$%04X", w);
        substitute_once(tpl, "a16", buf);
    }
    if (tpl.find("d8") != std::string::npos) {
        std::snprintf(buf, sizeof(buf), "$%02X", r.bytes[1]);
        substitute_once(tpl, "d8", buf);
    }
    if (tpl.find("a8") != std::string::npos) {
        std::snprintf(buf, sizeof(buf), "$FF%02X", r.bytes[1]);
        substitute_once(tpl, "a8", buf);
    }
    if (tpl.find("s8") != std::string::npos) {
        const auto signed_byte = static_cast<std::int8_t>(r.bytes[1]);
        // JR resolves to an absolute target (addr + 2 + signed offset).
        // LD HL, SP+s8 keeps the offset as a signed decimal — it's added to
        // SP at runtime, not to PC.
        if (tpl.rfind("JR", 0) == 0) {
            const auto target = static_cast<std::uint16_t>(addr + 2 + signed_byte);
            std::snprintf(buf, sizeof(buf), "$%04X", target);
        } else {
            std::snprintf(buf, sizeof(buf), "%+d", signed_byte);
        }
        substitute_once(tpl, "s8", buf);
    }

    r.text = std::move(tpl);
    return r;
}
