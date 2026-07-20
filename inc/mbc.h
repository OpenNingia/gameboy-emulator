#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
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
        std::uint8_t type;      // cartridge type byte (header $0147)
        std::uint16_t rom_bank; // currently mapped bank at $4000-$7FFF (MBC5 goes up to 511)
        std::uint8_t ram_bank;  // currently mapped RAM bank at $A000-$BFFF
        bool ram_enabled;       // RAM gate (always true for carts wired with RAM and no enable line)
        std::uint8_t mode;      // MBC1 advanced-banking mode bit (0 = ROM banking, 1 = RAM/upper-bits)
    };

    // CGB compatibility classification derived from the header byte at $0143.
    // The flag reflects the cartridge developer's intent, not what the
    // machine is doing — see classify_cgb_flag() for the bit pattern.
    enum class cgb_support : std::uint8_t {
        none,     // DMG cartridge (bit 7 clear). On real CGB hardware the boot
                  // ROM may auto-assign a colorization palette; we do not.
        compat,   // 0x80: CGB-enhanced, also runs on DMG with a fallback path.
        cgb_only, // 0xC0: CGB-only; a real DMG cannot run it.
    };

    // Classify the raw header byte at $0143. Only bit 7 distinguishes
    // DMG-only from CGB-aware; bit 6 (combined with bit 7) is what tags a
    // cartridge "CGB-only". Pan Docs notes the legacy values 0x84/0x88
    // (PGB mode) — they have bit 7 clear and we treat them as DMG.
    inline cgb_support classify_cgb_flag(std::uint8_t flag) {
        if ((flag & 0xC0) == 0xC0)
            return cgb_support::cgb_only;
        if (flag & 0x80)
            return cgb_support::compat;
        return cgb_support::none;
    }

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

        // CGB compatibility flag from header $0143. Static per cartridge —
        // captured at construction time. Drives the DMG-vs-CGB model
        // selection in core::load(); also surfaced in the MBC panel.
        virtual std::uint8_t cgb_flag() const = 0;

        // Restore banking state to power-on defaults.  Cartridge ROM bytes
        // are preserved (they're the immutable game data); RAM contents are
        // implementation-defined — for now no implementation clears RAM, so
        // a future battery-save layer can read it back after a reset.
        // Default is a no-op for cartridges with no banking state (no_mbc).
        virtual void reset() {}

        // True for cartridge types that carry a battery on the SRAM (or RTC)
        // line — i.e. types 0x03, 0x06, 0x09, 0x0D, 0x0F, 0x10, 0x13, 0x1B,
        // 0x1E, 0x22, 0xFF.  Drives whether the app should look for / write
        // a .sav for this cart.  Default false (covers no_mbc(0x00/0x08)
        // and the bare mbc1/2/3/5 variants without BATTERY).
        virtual bool has_battery() const { return false; }

        // Cartridge SRAM as a contiguous byte span — flat across banks, in
        // the same layout it would have on a real battery-backed cart
        // (compatible with BGB/mGBA/SameBoy .sav files at the SRAM level).
        // Empty span for carts with no external RAM.  MBC2's 512-nibble
        // built-in RAM is surfaced as 512 bytes here (low nibbles only,
        // upper nibbles zero) — the de facto convention used by other
        // emulators.
        virtual std::span<const std::uint8_t> ram_data() const { return {}; }

        // Restore SRAM from a previously dumped image.  Sizes that don't
        // match `ram_data().size()` are ignored (logged at the call site).
        virtual void ram_load(std::span<const std::uint8_t> /*src*/) {}

        // Sticky dirty flag — set on every write to $A000-$BFFF that
        // actually mutates RAM.  Read by the periodic flush.  Cleared via
        // ram_clear_dirty() once the data has been persisted.
        virtual bool ram_dirty() const { return false; }
        virtual void ram_clear_dirty() {}

        // RTC state, in the 48-byte BESS layout (block payload, no header).
        // nullopt for carts without an RTC chip (everyone except MBC3
        // TIMER variants, types 0x0F / 0x10).  Layout follows SameBoy
        // BESS.md: 0x00-0x10 live S/M/H/DL/DH (1 byte + 3 padding each),
        // 0x14-0x24 latched, 0x28-0x2F unix timestamp (int64 LE).
        virtual std::optional<std::array<std::uint8_t, 48>> rtc_blob() const { return std::nullopt; }
        virtual void rtc_load_blob(std::span<const std::uint8_t> /*src*/) {}
    };

    // Derive the BATTERY bit from the raw cartridge type byte ($0147).
    // Free function so callers (Application's load-time hookup) can check
    // it before an mbc instance even exists.
    bool cartridge_type_has_battery(std::uint8_t type);

    // Build the appropriate MBC for a freshly-loaded ROM image, looking at
    // the cartridge header (type 0x0147, ROM size 0x0148, RAM size 0x0149).
    // Throws gbemu_exception if the cartridge type is not supported.
    std::unique_ptr<mbc> make_mbc(std::vector<std::uint8_t> rom);

} // namespace gbemu
