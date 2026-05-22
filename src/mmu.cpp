#include <mmu.h>

std::uint8_t gbemu::mmu::read_u8(std::uint16_t addr) const
{
	switch (addr & 0xF000) {
		// the first 256 bytes can be either the bios
		// or the first bank of cardrige
		// depending on the 'bios_accessible' flag
	case 0x0000: {
		if (bios_accessible && addr < 0x100) {
			return bios[addr];
		}

		return rom0[addr];
	}
			   // first rom bank
	case 0x1000:
	case 0x2000:
	case 0x3000:
		return rom0[addr];

		// second rom bank
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		return rom1[addr - 0x4000];

		// gpu vram
	case 0x8000:
	case 0x9000:
		return vram[addr - 0x8000];

		// external ram
	case 0xA000:
	case 0xB000:
		return eram[addr - 0xA000];

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
		throw gbemu_exception{ "Invalid address!" };
	}
}

std::int8_t gbemu::mmu::read_i8(std::uint16_t addr) const
{
	return static_cast<std::int8_t>(read_u8(addr));
}

void gbemu::mmu::write_u8(std::uint16_t addr, std::uint8_t val)
{
	switch (addr & 0xF000) {
		// first rom bank | bios
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
		// nothing to do, but don't throw an exception since some cartridges use this area for bank switching
		break;

		// second rom bank
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		// nothing to do, but don't throw an exception since some cartridges use this area for bank switching
		break;

		// gpu vram
	case 0x8000:
	case 0x9000:
		vram[addr - 0x8000] = val;
		break;

		// external ram
	case 0xA000:
	case 0xB000:
		eram[addr - 0xA000] = val;
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
			throw gbemu_exception{ "Not addressable!" };
		// memory mapped i/o
		else if (addr < 0xFF80) {
			mmio[addr - 0xFF00] = val;

			// stampa a console i dati scritti nella porta seriale
			if (addr == 0xFF02 && val == 0x81) {
				std::putchar(static_cast<char>(read_u8(0xFF01)));
				std::fflush(stdout);
				mmio[0xFF02 - 0xFF00] = 0x01;   // <-- clear bit 7 ("transfer done")
				// opzionale: set IF.3 per generare interrupt seriale
			}
		}
		// zram
		else
			zram[addr - 0xFF80] = val;
		break;
	default:
		throw gbemu_exception{ "Invalid address!" };
	}
}

void gbemu::mmu::initialize_registers()
{
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


	// TODO...
}

void gbemu::mmu::tick_io_stub()
{
	if (++ppu_stub_counter < ppu_stub_step) return;
	ppu_stub_counter = 0;

	std::uint8_t ly = hwr_ly();
	ly = static_cast<std::uint8_t>((ly + 1) % 154);
	// LY lives in mmio[] — write directly so we don't recurse through the
	// FF02 serial shim in write_u8 and don't pay another dispatch.
	mmio[0xFF44 - 0xFF00] = ly;
}