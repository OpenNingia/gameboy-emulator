#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace gbemu {

    // Cartridge type byte at header $0147. Values follow Pan Docs.
    enum class cartridge_type : std::uint8_t {
        rom_only = 0x00,
        mbc1 = 0x01,
        mbc1_ram = 0x02,
        mbc1_ram_battery = 0x03,
        mbc2 = 0x05,
        mbc2_battery = 0x06,
        rom_ram = 0x08,
        rom_ram_battery = 0x09,
        mmm01 = 0x0B,
        mmm01_ram = 0x0C,
        mmm01_ram_battery = 0x0D,
        mbc3_timer_battery = 0x0F,
        mbc3_timer_ram_battery = 0x10,
        mbc3 = 0x11,
        mbc3_ram = 0x12,
        mbc3_ram_battery = 0x13,
        mbc5 = 0x19,
        mbc5_ram = 0x1A,
        mbc5_ram_battery = 0x1B,
        mbc5_rumble = 0x1C,
        mbc5_rumble_ram = 0x1D,
        mbc5_rumble_ram_battery = 0x1E,
        mbc6 = 0x20,
        mbc7_sensor_rumble_ram_battery = 0x22,
        pocket_camera = 0xFC,
        bandai_tama5 = 0xFD,
        huc3 = 0xFE,
        huc1_ram_battery = 0xFF,
    };

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

        // Restore banking state to power-on defaults.  Cartridge ROM bytes
        // are preserved (they're the immutable game data); RAM contents are
        // implementation-defined — for now no implementation clears RAM, so
        // a future battery-save layer can read it back after a reset.
        // Default is a no-op for cartridges with no banking state (no_mbc).
        virtual void reset() {}
    };

    // Build the appropriate MBC for a freshly-loaded ROM image, looking at
    // the cartridge header (type 0x0147, ROM size 0x0148, RAM size 0x0149).
    // Throws gbemu_exception if the cartridge type is not supported.
    std::unique_ptr<mbc> make_mbc(std::vector<std::uint8_t> rom);

} // namespace gbemu
