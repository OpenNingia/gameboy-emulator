#pragma once
#ifndef _H_DISASM_H_
#    define _H_DISASM_H_

#    include <array>
#    include <cstdint>
#    include <string>

namespace gbemu {
    struct mmu;

    struct disasm_result {
        std::array<std::uint8_t, 3> bytes{}; // raw bytes; only [0..length) valid
        std::uint8_t length{0};              // total instruction length
        std::string text{};                  // formatted mnemonic
    };

    // Decode one instruction at `addr`.  Reads up to 3 bytes through `m`
    // (consult `length` to know how many are meaningful).  Placeholder
    // substitution (d8/d16/s8/a8/a16) is performed here; JR targets are
    // resolved to absolute addresses.  Pure function — no debugger state.
    disasm_result disasm_one(std::uint16_t addr, const mmu& m);

} // namespace gbemu

#endif // _H_DISASM_H_
