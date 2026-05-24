#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace gbemu {

    // Snapshot of the MBC's banking state, surfaced to the debugger / MBC panel.
    struct mbc_debug_state {
        std::uint8_t type;     // cartridge type byte (header $0147)
        std::uint8_t rom_bank; // currently mapped bank at $4000-$7FFF
        std::uint8_t ram_bank; // currently mapped RAM bank at $A000-$BFFF
        bool ram_enabled;      // RAM gate (always true for carts wired with RAM and no enable line)
        std::uint8_t mode;     // MBC1 advanced-banking mode bit (0 = ROM banking, 1 = RAM/upper-bits)
    };

    // Memory Bank Controller interface.
    //
    // Owns the cartridge ROM bytes and any cartridge RAM.  The MMU forwards
    // every cartridge-range access (0x0000-0x7FFF and 0xA000-0xBFFF) here and
    // the concrete implementation is responsible for banking, RAM enable
    // gating, and any quirks of the underlying chip.
    struct mbc {
        virtual ~mbc() = default;

        // Read a byte from a cartridge address.  Addresses outside the
        // cartridge ranges are undefined for the implementation.
        virtual std::uint8_t read(std::uint16_t addr) const = 0;

        // Write a byte to a cartridge address.  Writes to the ROM range are
        // typically used to drive bank/control registers.
        virtual void write(std::uint16_t addr, std::uint8_t val) = 0;

        // Debug snapshot of the current banking state — read by the debugger
        // (`dump mbc`) and by the ImGui MBC panel.  Pure introspection;
        // implementations must not mutate state here.
        virtual mbc_debug_state debug_state() const = 0;
    };

    // Build the appropriate MBC for a freshly-loaded ROM image, looking at
    // the cartridge header (type 0x0147, ROM size 0x0148, RAM size 0x0149).
    // Throws gbemu_exception if the cartridge type is not supported.
    std::unique_ptr<mbc> make_mbc(std::vector<std::uint8_t> rom);

} // namespace gbemu
