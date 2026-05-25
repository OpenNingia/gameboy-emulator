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

    // VBK ($FF4F) — VRAM bank select.  CGB only; on DMG the write is ignored
    // and the bank stays at 0.  Only bit 0 is writable.
    add_mmio_write_handler(gb::io::VBK, [this](std::uint8_t v) {
        if (cgb_mode_)
            vram_bank_ = static_cast<std::uint8_t>(v & 0x01);
    });

    // SVBK ($FF70) — WRAM bank select for $D000-$DFFF (and the corresponding
    // echo-RAM window).  CGB only; on DMG the write is ignored.  Three bits
    // are writable; raw=0 maps to bank 1 in hardware, banks 1-7 map to 1-7.
    add_mmio_write_handler(gb::io::SVBK, [this](std::uint8_t v) {
        if (cgb_mode_) {
            const std::uint8_t b = static_cast<std::uint8_t>(v & 0x07);
            wram_bank_ = (b == 0) ? std::uint8_t{1} : b;
        }
    });

    // KEY1 ($FF4D) — double-speed control.  CGB only; on DMG the write is
    // ignored.  Only bit 0 (prepare speed switch) is software-writable; bit 7
    // (current speed) is hardware-driven by a successful STOP transition,
    // which we don't model yet (step 8) — preserving its current value keeps
    // the slot ready for that work.  write_u8 already stored the raw byte in
    // mmio_[KEY1]; rewrite it here to keep only the legal bits.
    add_mmio_write_handler(gb::io::KEY1, [this](std::uint8_t v) {
        if (cgb_mode_) {
            auto& slot = mmio_[gb::io_offset(gb::io::KEY1)];
            slot = static_cast<std::uint8_t>((slot & 0x80) | (v & 0x01));
        }
    });

    // BCPS / BCPD ($FF68 / $FF69) — CGB BG palette index / data.
    // BCPD stores the byte into bg_palette_ram_[BCPS & 0x3F]; if BCPS bit 7
    // is set, the index (bits 0-5) post-increments with wrap at 64. The byte
    // ends up in mmio_[BCPD] too (write_u8 already stored it), but the
    // mmio slot is never read — BCPD reads route through mmio_read_masked
    // back to bg_palette_ram_.  Both registers are inert on DMG.
    add_mmio_write_handler(gb::io::BCPD, [this](std::uint8_t v) {
        if (!cgb_mode_)
            return;
        auto& bcps = mmio_[gb::io_offset(gb::io::BCPS)];
        bg_palette_ram_[bcps & 0x3F] = v;
        if (bcps & 0x80)
            bcps = static_cast<std::uint8_t>(0x80 | ((bcps + 1) & 0x3F));
    });

    // OCPS / OCPD ($FF6A / $FF6B) — CGB OBJ palette index / data.  Same
    // shape as BCPS/BCPD, just over obj_palette_ram_.
    add_mmio_write_handler(gb::io::OCPD, [this](std::uint8_t v) {
        if (!cgb_mode_)
            return;
        auto& ocps = mmio_[gb::io_offset(gb::io::OCPS)];
        obj_palette_ram_[ocps & 0x3F] = v;
        if (ocps & 0x80)
            ocps = static_cast<std::uint8_t>(0x80 | ((ocps + 1) & 0x3F));
    });
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

            // gpu vram — bank is the VBK latch on CGB, always 0 on DMG.
        case 0x8000:
        case 0x9000:
            return vram_[vram_bank_][addr - gb::VRAM_BASE];

            // cartridge external ram
        case 0xA000:
        case 0xB000:
            return cart_ ? cart_->read(addr) : gb::OPEN_BUS;

            // working ram — $C000-$CFFF is the fixed bank-0 window, $D000-$DFFF
            // is the SVBK-selected bank (1-7 on CGB; stays at 1 on DMG so the
            // dispatch reproduces the flat 8 KB layout).
        case 0xC000:
            return wram_[0][addr - gb::WRAM_BASE];
        case 0xD000:
            return wram_[wram_bank_][addr - 0xD000];

            // echo ram — mirrors WRAM with the same bank-0 / SVBK split:
            // $E000-$EFFF aliases $C000-$CFFF, $F000-$FDFF aliases $D000-$FDDF.
        case 0xE000:
            return wram_[0][addr - gb::ECHO_BASE];

        case 0xF000:
            // echo ram (upper half — banked window)
            if (addr < gb::OAM_BASE)
                return wram_[wram_bank_][addr - 0xF000];
            // sprite ram
            if (addr < gb::OAM_BASE + gb::OAM_TOTAL_BYTES)
                return oam_[addr - gb::OAM_BASE];
            // black hole
            if (addr < gb::io::BASE)
                return 0;
            // memory mapped i/o (IF mask + CGB-only register masks)
            if (addr < gb::HRAM_BASE)
                return mmio_read_masked(addr);
            // hram (zero-page)
            return hram_[addr - gb::HRAM_BASE];
        default:
            throw gbemu_exception{"Invalid address!"};
    }
}

std::int8_t gbemu::mmu::read_i8(std::uint16_t addr) const {
    return static_cast<std::int8_t>(read_u8(addr));
}

std::uint8_t gbemu::mmu::mmio_read_masked(std::uint16_t addr) const {
    const auto off = gb::io_offset(addr);
    const auto v = mmio_[off];

    // IF ($FF0F) bits 5-7 are unimplemented in hardware and read back as 1
    // (open-bus / pull-up). Blargg's halt_bug.gb checksums them.
    if (addr == gb::io::IF)
        return static_cast<std::uint8_t>(v | gb::irq_bit::if_unimpl_high);

    // CGB-only registers — on DMG the whole block reads open-bus.  This is
    // load-bearing for KEY1 specifically: Blargg cpu_fast probes it to skip
    // the double-speed path, and a write earlier in the run would otherwise
    // leak back through this read (see project_blargg_runtime memory).
    if (!cgb_mode_) {
        switch (addr) {
            case gb::io::KEY1:
            case gb::io::VBK:
            case gb::io::HDMA1:
            case gb::io::HDMA2:
            case gb::io::HDMA3:
            case gb::io::HDMA4:
            case gb::io::HDMA5:
            case gb::io::BCPS:
            case gb::io::BCPD:
            case gb::io::OCPS:
            case gb::io::OCPD:
            case gb::io::SVBK:
                return gb::OPEN_BUS;
        }
        return v;
    }

    // CGB read masks for registers with unimplemented bits that pull to 1.
    // HDMA1-5 still wait on the HDMA new-code subsystem — until then they
    // return raw storage, which Blargg's CGB suites tolerate (0 after reset).
    switch (addr) {
        case gb::io::KEY1:
            // bits 0 (prepare) and 7 (current speed) live in storage; 1-6 pull-up.
            return static_cast<std::uint8_t>(0x7E | (v & 0x81));
        case gb::io::VBK:
            // only bit 0 carries the bank; the rest of the byte pulls to 1.
            return static_cast<std::uint8_t>(0xFE | vram_bank_);
        case gb::io::SVBK:
            // Pan Docs: read returns the raw write ANDed with 0x07; bits 3-7 pull.
            return static_cast<std::uint8_t>(0xF8 | (v & 0x07));
        case gb::io::BCPS:
        case gb::io::OCPS:
            // bit 7 (auto-increment) + bits 0-5 (index) are real; bit 6 pulls high.
            return static_cast<std::uint8_t>(0x40 | (v & 0xBF));
        case gb::io::BCPD:
            return bg_palette_ram_[mmio_[gb::io_offset(gb::io::BCPS)] & 0x3F];
        case gb::io::OCPD:
            return obj_palette_ram_[mmio_[gb::io_offset(gb::io::OCPS)] & 0x3F];
    }
    return v;
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

            // gpu vram — bank is the VBK latch on CGB, always 0 on DMG.
        case 0x8000:
        case 0x9000:
            vram_[vram_bank_][addr - gb::VRAM_BASE] = val;
            break;

            // cartridge external ram
        case 0xA000:
        case 0xB000:
            if (cart_)
                cart_->write(addr, val);
            break;

            // working ram — see read_u8 for the bank split.
        case 0xC000:
            wram_[0][addr - gb::WRAM_BASE] = val;
            break;
        case 0xD000:
            wram_[wram_bank_][addr - 0xD000] = val;
            break;

            // echo ram (mirrors WRAM with the same bank split)
        case 0xE000:
            wram_[0][addr - gb::ECHO_BASE] = val;
            break;

        case 0xF000:
            // echo ram (upper half — banked window)
            if (addr < gb::OAM_BASE)
                wram_[wram_bank_][addr - 0xF000] = val;
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

std::uint8_t gbemu::mmu::vram_read(std::uint16_t off, std::uint8_t bank) const {
    return vram_[bank][off];
}

void gbemu::mmu::vram_write(std::uint16_t off, std::uint8_t val, std::uint8_t bank) {
    vram_[bank][off] = val;
}

std::span<const std::uint8_t> gbemu::mmu::vram_bank(std::uint8_t bank) const {
    return {vram_[bank].data(), vram_[bank].size()};
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
    for (auto& b : vram_)
        b.fill(0);
    for (auto& b : wram_)
        b.fill(0);
    vram_bank_ = 0;
    wram_bank_ = 1;
    oam_.fill(0);
    mmio_.fill(0);
    hram_.fill(0);
    bg_palette_ram_.fill(0);
    obj_palette_ram_.fill(0);
    // KEY1 (and the rest of the CGB I/O block) no longer needs a 0xFF reseed
    // here: mmio_read_masked() returns OPEN_BUS on DMG and the 0x7E mask on
    // CGB regardless of the underlying mmio_ storage.  cgb_mode_ is a
    // cartridge property and survives the reset; the VRAM/WRAM bank latches
    // and bios_accessible_ are re-armed below.
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
