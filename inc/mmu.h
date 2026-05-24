#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

#include <absl/container/inlined_vector.h>
#include <exc.hpp>
#include <mbc.h>

#define DEF_HWREG(x, addr)         \
    std::uint8_t hwr_##x() const { \
        return read_u8(addr);      \
    }                              \
    void hwr_##x(std::uint8_t v) { \
        write_u8(addr, v);         \
    }

#define DEF_HWREG_NP(x, addr) \
    std::uint8_t x() const {  \
        return read_u8(addr); \
    }                         \
    void x(std::uint8_t v) {  \
        write_u8(addr, v);    \
    }

namespace gbemu {

    template <std::size_t N>
    using ram_t = std::array<std::uint8_t, N>;

    struct mmu {
        mmu();

        using mmio_write_fn = std::function<void(std::uint8_t)>;

        // Install the cartridge.  Called once after the ROM has been read
        // off disk and the appropriate mbc subclass has been instantiated.
        void attach_cartridge(std::unique_ptr<mbc> c) { cart_ = std::move(c); }
        // Returns the attached cartridge or nullptr if none.  The MMU is the
        // sole owner; callers must not retain the pointer across an
        // attach_cartridge call.
        const mbc* cart() const { return cart_.get(); }

        // Bus-side access — walks the full memory map dispatch and fires any
        // registered MMIO write handlers.  This is the path the CPU and
        // debug observers use.
        std::uint8_t read_u8(std::uint16_t addr) const;
        std::int8_t read_i8(std::uint16_t addr) const;
        void write_u8(std::uint16_t addr, std::uint8_t val);

        // Register a write-side hook for a single MMIO byte (0xFF00-0xFF7F).
        // Multiple handlers may be registered for the same address; they are
        // invoked in registration order after the byte has been stored in
        // mmio[].  Used by hardware subsystems (serial, dma, timer, apu, ...)
        // and by debug observers (write-watchpoints, serial taps).
        void add_mmio_write_handler(std::uint16_t addr, mmio_write_fn fn);

        std::uint16_t read_u16(std::uint16_t addr) { return read_u8(addr) + (read_u8(addr + 1) << 8); }

        void write_u16(std::uint16_t addr, std::uint16_t val) {
            write_u8(addr + 1, (val & 0xFF00) >> 8);
            write_u8(addr, val & 0x00FF);
        }

        // ------------------------------------------------------------------
        // Subsystem-internal accessors.
        //
        // These bypass the bus: no MMIO write handler fans out, no BIOS
        // overlay is consulted, no IF read-mask is applied.  Chips use them
        // to poke their own backing storage without recursing back into
        // write_u8 (and therefore back into their own handlers).
        //
        // The `bank` parameter on the VRAM accessors is reserved for the
        // CGB VBK ($FF4F) bank select; on DMG it must remain 0.
        // ------------------------------------------------------------------

        // VRAM ($8000-$9FFF).  `off` is the offset from VRAM_BASE.
        std::uint8_t vram_read(std::uint16_t off, std::uint8_t bank = 0) const;
        void vram_write(std::uint16_t off, std::uint8_t val, std::uint8_t bank = 0);
        std::span<const std::uint8_t> vram_bank(std::uint8_t bank = 0) const;

        // OAM (sprite RAM, $FE00-$FE9F).  `off` in [0, OAM_TOTAL_BYTES).
        std::uint8_t oam_read(std::uint8_t off) const;
        void oam_write(std::uint8_t off, std::uint8_t val);

        // Direct I/O region access ($FF00-$FF7F) for chip-internal updates
        // that must not re-enter write_u8 — e.g. the timer's per-cycle DIV
        // increment, the joypad's P1/IF latch refresh, the APU's read-mask
        // application.
        std::uint8_t io_read(std::uint16_t addr) const;
        void io_store(std::uint16_t addr, std::uint8_t val);

        // Wave RAM ($FF30-$FF3F) view for the APU's CH3 sample pump.
        std::span<const std::uint8_t> wave_ram() const;

        // BIOS overlay.  load_bios copies the ROM image into the internal
        // buffer and arms the overlay so $0000-$00FF subsequently returns
        // BIOS bytes instead of cartridge bank 0; on a nonzero write to
        // $FF50 the MMU disarms the overlay (one-shot until reset on DMG).
        void load_bios(std::span<const std::uint8_t> data);
        bool bios_active() const { return bios_accessible_; }
        std::size_t bios_size() const { return bios_.size(); }

        // hardware registers
        DEF_HWREG(p1, 0xFF00);
        DEF_HWREG(sb, 0xFF01);
        DEF_HWREG(sc, 0xFF02);
        DEF_HWREG(div, 0xFF04);
        DEF_HWREG(tima, 0xFF05);
        DEF_HWREG(tma, 0xFF06);
        DEF_HWREG(tac, 0xFF07);
        DEF_HWREG_NP(hwr_if, 0xFF0F);
        // TODO NR10-52

        DEF_HWREG(lcdc, 0xFF40);
        DEF_HWREG_NP(hwr_stat, 0xFF41);
        DEF_HWREG(scy, 0xFF42);
        DEF_HWREG(scx, 0xFF43);
        DEF_HWREG(ly, 0xFF44);
        DEF_HWREG(lyc, 0xFF45);
        DEF_HWREG(dma, 0xFF46);
        DEF_HWREG(bgp, 0xFF47);
        DEF_HWREG(obp0, 0xFF48);
        DEF_HWREG(obp1, 0xFF49);
        DEF_HWREG(wy, 0xFF4A);
        DEF_HWREG(wx, 0xFF4B);
        DEF_HWREG_NP(hwr_ie, 0xFFFF);

        // boot sequence
        void initialize_registers();

        // Wipe all RAM regions (VRAM, WRAM, OAM, MMIO, HRAM) back to zero and
        // re-arm the BIOS overlay if a BIOS image was previously loaded.  The
        // cartridge is preserved (its `reset()` is invoked to clear banking
        // state) and so is the table of registered MMIO write handlers — they
        // live in the MMU but are owned by subsystem constructors, so wiping
        // them here would silently break IRQ routing, joypad latch, etc.
        void reset();

    private:
        bool bios_accessible_{false};
        // True once a BIOS image has been load_bios()'d. Reset uses it to
        // decide whether to re-arm the overlay (a Reset that re-runs BIOS is
        // closer to a real power cycle than one that skips it).
        bool bios_loaded_{false};

        // bios code 0x0000 -> 0x00FF
        ram_t<0x0100> bios_{};
        // cartridge (ROM + external RAM); owns its own bytes and handles
        // banking.  Installed by core::load(rom_file&) via attach_cartridge.
        std::unique_ptr<mbc> cart_{};
        // gpu vram 0x8000 -> 0x9FFF
        ram_t<0x2000> vram_{};
        // working ram 0xC000 -> 0xDFFF (mirrored at 0xE000 -> 0xFDFF)
        ram_t<0x2000> wram_{};
        // sprite ram (OAM) 0xFE00 -> 0xFE9F
        ram_t<0x00A0> oam_{};
        // memory-mapped I/O region 0xFF00 -> 0xFF7F
        ram_t<0x0080> mmio_{};
        // high RAM / zero-page 0xFF80 -> 0xFFFE (+ IE at 0xFFFF)
        ram_t<0x0080> hram_{};

        // Inline N=2 covers the typical occupancy (hardware emulation
        // handler + at most one or two debug observers); rarer 3+ cases
        // spill to heap.
        std::array<absl::InlinedVector<mmio_write_fn, 2>, 0x80> mmio_write_handlers_{};
    };
} // namespace gbemu
