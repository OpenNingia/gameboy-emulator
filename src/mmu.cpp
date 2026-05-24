#include <mmu.h>

gbemu::mmu::mmu() {
    // Boot ROM disable lives in MMU itself: writing nonzero to 0xFF50 unmaps
    // the BIOS from 0x0000-0x00FF. On DMG this is one-shot until reset.
    add_mmio_write_handler(0xFF50, [this](std::uint8_t v) {
        if (v != 0)
            bios_accessible = false;
    });

    // CGB-only registers — on DMG these read back as open-bus 0xFF.  Blargg's
    // cpu_instrs runtime probes KEY1 ($FF4D) in cpu_fast to decide whether the
    // CPU is already in double-speed mode; without 0xFF here the probe falls
    // through to a STOP that would otherwise lock up the test on DMG.
    // TODO(CGB): when CGB support is added this must become 0x7E on CGB, and
    // KEY1 writes must respect bit 0 (prepare speed switch) being the only
    // writable bit.
    mmio[0xFF4D - 0xFF00] = 0xFF;
}

void gbemu::mmu::add_mmio_write_handler(std::uint16_t addr, mmio_write_fn fn) {
    mmio_write_handlers[addr - 0xFF00].push_back(std::move(fn));
}

std::uint8_t gbemu::mmu::read_u8(std::uint16_t addr) const {
    switch (addr & 0xF000) {
            // the first 256 bytes can be either the bios
            // or the first bank of cardrige
            // depending on the 'bios_accessible' flag
        case 0x0000: {
            if (bios_accessible && addr < 0x100) {
                return bios[addr];
            }

            return cart ? cart->read(addr) : std::uint8_t{0xFF};
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
            return cart ? cart->read(addr) : std::uint8_t{0xFF};

            // gpu vram
        case 0x8000:
        case 0x9000:
            return vram[addr - 0x8000];

            // cartridge external ram
        case 0xA000:
        case 0xB000:
            return cart ? cart->read(addr) : std::uint8_t{0xFF};

            // working ram
        case 0xC000:
        case 0xD000:
            return wram[addr - 0xC000];

            // echo ram
        case 0xE000:
            return wram[addr - 0xE000];

        case 0xF000:
            // echo ram
            if (addr < 0xFE00)
                return wram[addr - 0xE000];
            // sprite ram
            if (addr < 0xFEA0)
                return sram[addr - 0xFE00];
            // black hole
            if (addr < 0xFF00)
                return 0;
            // memory mapped i/o
            if (addr < 0xFF80)
                return mmio[addr - 0xFF00];
            // zram
            return zram[addr - 0xFF80];
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
            if (cart)
                cart->write(addr, val);
            break;

            // gpu vram
        case 0x8000:
        case 0x9000:
            vram[addr - 0x8000] = val;
            break;

            // cartridge external ram
        case 0xA000:
        case 0xB000:
            if (cart)
                cart->write(addr, val);
            break;

            // working ram
        case 0xC000:
        case 0xD000:
            wram[addr - 0xC000] = val;
            break;

            // echo ram
        case 0xE000:
            wram[addr - 0xE000] = val;
            break;

        case 0xF000:
            // echo ram
            if (addr < 0xFE00)
                wram[addr - 0xE000] = val;
            // sprite ram
            else if (addr < 0xFEA0)
                sram[addr - 0xFE00] = val;
            // black hole
            else if (addr < 0xFF00)
                throw gbemu_exception{"Not addressable!"};
            // memory mapped i/o
            else if (addr < 0xFF80) {
                mmio[addr - 0xFF00] = val;
                for (auto& fn : mmio_write_handlers[addr - 0xFF00]) {
                    fn(val);
                }
            }
            // zram
            else
                zram[addr - 0xFF80] = val;
            break;
        default:
            throw gbemu_exception{"Invalid address!"};
    }
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
