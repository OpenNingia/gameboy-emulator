#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>

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

        bool bios_accessible{false};

        // bios code 0x0000 -> 0x00FF
        ram_t<0x0100> bios{};
        // cartridge (ROM + external RAM); owns its own bytes and handles
        // banking.  Installed by core::load(rom_file&) via attach_cartridge.
        std::unique_ptr<mbc> cart{};
        // gpu vram 0x8000 -> 0x9FFF
        ram_t<0x2000> vram{};
        // working ram 0xC000 -> 0xDFFF
        ram_t<0x2000> wram{};
        // echo ram (addressed by code) 0xE000 -> 0xFDFF
        // sprite ram
        ram_t<0x00A0> sram{};
        // mmio
        ram_t<0x0080> mmio{};
        // zero-page ram
        ram_t<0x0080> zram{};

        // Install the cartridge.  Called once after the ROM has been read
        // off disk and the appropriate mbc subclass has been instantiated.
        void attach_cartridge(std::unique_ptr<mbc> c) { cart = std::move(c); }

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

    private:
        unsigned ppu_stub_counter{0};
        static constexpr unsigned ppu_stub_step{32};

        // Inline N=2 covers the typical occupancy (hardware emulation
        // handler + at most one or two debug observers); rarer 3+ cases
        // spill to heap.
        std::array<absl::InlinedVector<mmio_write_fn, 2>, 0x80> mmio_write_handlers{};
    };
} // namespace gbemu
