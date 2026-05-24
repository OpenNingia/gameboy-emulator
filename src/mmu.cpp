#include <cstring>

#include <gb_layout.h>
#include <mmu.h>

gbemu::mmu::mmu() {
    // Boot ROM disable lives in MMU itself: writing nonzero to BOOT_OFF unmaps
    // the BIOS from $0000-$00FF. On DMG this is one-shot until reset.
    add_mmio_write_handler(gb::io::BOOT_OFF, [this](std::uint8_t v) {
        if (v != 0)
            bios_accessible_ = false;
    });

    // CGB-only registers — on DMG these read back as open-bus 0xFF.  Blargg's
    // cpu_instrs runtime probes KEY1 in cpu_fast to decide whether the CPU is
    // already in double-speed mode; without 0xFF here the probe falls through
    // to a STOP that would otherwise lock up the test on DMG.
    // TODO(CGB): when CGB support is added this must become 0x7E on CGB, and
    // KEY1 writes must respect bit 0 (prepare speed switch) being the only
    // writable bit.
    mmio_[gb::io_offset(gb::io::KEY1)] = gb::OPEN_BUS;
}

void gbemu::mmu::add_mmio_write_handler(std::uint16_t addr, mmio_write_fn fn) {
    mmio_write_handlers_[gb::io_offset(addr)].push_back(std::move(fn));
}

std::uint8_t gbemu::mmu::read_u8(std::uint16_t addr) const {
    switch (addr & 0xF000) {
            // the first 256 bytes can be either the bios
            // or the first bank of cardrige
            // depending on the 'bios_accessible_' flag
        case 0x0000: {
            if (bios_accessible_ && addr < 0x100) {
                return bios_[addr];
            }

            return cart_ ? cart_->read(addr) : gb::OPEN_BUS;
        }
            // cartridge ROM (bank 0 fixed slot)
        case 0x1000:
        case 0x2000:
        case 0x3000:
            // cartridge ROM (banked slot)
        case 0x4000:
        case 0x5000:
        case 0x6000:
        case 0x7000:
            return cart_ ? cart_->read(addr) : gb::OPEN_BUS;

            // gpu vram
        case 0x8000:
        case 0x9000:
            return vram_[addr - gb::VRAM_BASE];

            // cartridge external ram
        case 0xA000:
        case 0xB000:
            return cart_ ? cart_->read(addr) : gb::OPEN_BUS;

            // working ram
        case 0xC000:
        case 0xD000:
            return wram_[addr - gb::WRAM_BASE];

            // echo ram
        case 0xE000:
            return wram_[addr - gb::ECHO_BASE];

        case 0xF000:
            // echo ram
            if (addr < gb::OAM_BASE)
                return wram_[addr - gb::ECHO_BASE];
            // sprite ram
            if (addr < gb::OAM_BASE + gb::OAM_TOTAL_BYTES)
                return oam_[addr - gb::OAM_BASE];
            // black hole
            if (addr < gb::io::BASE)
                return 0;
            // memory mapped i/o
            if (addr < gb::HRAM_BASE) {
                auto v = mmio_[gb::io_offset(addr)];
                // IF ($FF0F) bits 5-7 are unimplemented in hardware and read
                // back as 1 (open-bus / pull-up). Blargg's halt_bug.gb depends
                // on this: it prints IF after the test and the CRC includes
                // those high bits. Without the mask we'd produce e.g.
                // "01 10 11 ..." where a real DMG shows "01 10 F1 ...".
                if (addr == gb::io::IF)
                    return static_cast<std::uint8_t>(v | gb::irq_bit::if_unimpl_high);
                return v;
            }
            // hram (zero-page)
            return hram_[addr - gb::HRAM_BASE];
        default:
            throw gbemu_exception{"Invalid address!"};
    }
}

std::int8_t gbemu::mmu::read_i8(std::uint16_t addr) const {
    return static_cast<std::int8_t>(read_u8(addr));
}

void gbemu::mmu::write_u8(std::uint16_t addr, std::uint8_t val) {
    switch (addr & 0xF000) {
            // cartridge ROM area — writes drive MBC control registers
        case 0x0000:
        case 0x1000:
        case 0x2000:
        case 0x3000:
        case 0x4000:
        case 0x5000:
        case 0x6000:
        case 0x7000:
            if (cart_)
                cart_->write(addr, val);
            break;

            // gpu vram
        case 0x8000:
        case 0x9000:
            vram_[addr - gb::VRAM_BASE] = val;
            break;

            // cartridge external ram
        case 0xA000:
        case 0xB000:
            if (cart_)
                cart_->write(addr, val);
            break;

            // working ram
        case 0xC000:
        case 0xD000:
            wram_[addr - gb::WRAM_BASE] = val;
            break;

            // echo ram
        case 0xE000:
            wram_[addr - gb::ECHO_BASE] = val;
            break;

        case 0xF000:
            // echo ram
            if (addr < gb::OAM_BASE)
                wram_[addr - gb::ECHO_BASE] = val;
            // sprite ram
            else if (addr < gb::OAM_BASE + gb::OAM_TOTAL_BYTES)
                oam_[addr - gb::OAM_BASE] = val;
            // "Not Usable" region $FEA0-$FEFF — on real DMG writes here are
            // silently ignored (modulo specific OAM-corruption side effects we
            // don't model). Several commercial ROMs (e.g. Tetris) hit it
            // during normal execution, so throwing is wrong: drop the write.
            else if (addr < gb::io::BASE)
                break;
            // memory mapped i/o
            else if (addr < gb::HRAM_BASE) {
                const auto off = gb::io_offset(addr);
                mmio_[off] = val;
                for (auto& fn : mmio_write_handlers_[off]) {
                    fn(val);
                }
            }
            // hram (zero-page)
            else
                hram_[addr - gb::HRAM_BASE] = val;
            break;
        default:
            throw gbemu_exception{"Invalid address!"};
    }
}

std::uint8_t gbemu::mmu::vram_read(std::uint16_t off, std::uint8_t /*bank*/) const {
    return vram_[off];
}

void gbemu::mmu::vram_write(std::uint16_t off, std::uint8_t val, std::uint8_t /*bank*/) {
    vram_[off] = val;
}

std::span<const std::uint8_t> gbemu::mmu::vram_bank(std::uint8_t /*bank*/) const {
    return {vram_.data(), vram_.size()};
}

std::uint8_t gbemu::mmu::oam_read(std::uint8_t off) const {
    return oam_[off];
}

void gbemu::mmu::oam_write(std::uint8_t off, std::uint8_t val) {
    oam_[off] = val;
}

std::uint8_t gbemu::mmu::io_read(std::uint16_t addr) const {
    return mmio_[gb::io_offset(addr)];
}

void gbemu::mmu::io_store(std::uint16_t addr, std::uint8_t val) {
    mmio_[gb::io_offset(addr)] = val;
}

std::span<const std::uint8_t> gbemu::mmu::wave_ram() const {
    return {mmio_.data() + gb::io_offset(gb::io::WAVE_RAM_BASE), gb::io::WAVE_RAM_END - gb::io::WAVE_RAM_BASE};
}

void gbemu::mmu::load_bios(std::span<const std::uint8_t> data) {
    const auto n = std::min(data.size(), bios_.size());
    std::memcpy(bios_.data(), data.data(), n);
    bios_accessible_ = true;
    bios_loaded_ = true;
}

void gbemu::mmu::reset() {
    vram_.fill(0);
    wram_.fill(0);
    oam_.fill(0);
    mmio_.fill(0);
    hram_.fill(0);
    // Repaint the KEY1 open-bus byte the ctor wrote — without this Blargg
    // cpu_fast's probe at PC=0x0150 would see 0x00 after a Reset and try a
    // STOP that locks up the test.
    mmio_[gb::io_offset(gb::io::KEY1)] = gb::OPEN_BUS;
    bios_accessible_ = bios_loaded_;
    if (cart_)
        cart_->reset();
}

void gbemu::mmu::initialize_registers() {
    hwr_p1(0xCF);
    hwr_sb(0x00);
    hwr_sc(0x7E);
    hwr_div(0xAB);
    hwr_tima(0x00);
    hwr_tma(0x00);
    hwr_tac(0xF8);
    hwr_if(0xE1);
    // TODO NR10-52

    hwr_lcdc(0x91);
    hwr_stat(0x85);
    hwr_scy(0x00);
    hwr_scx(0x00);
    hwr_ly(0x00);
    hwr_lyc(0x00);

    hwr_bgp(0xFC);
    hwr_obp0(0xFF);
    hwr_obp1(0xFF);
    hwr_wy(0x00);
    hwr_wx(0x00);
    hwr_ie(0x00);
    hwr_dma(0xFF);
}
