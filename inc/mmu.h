#pragma once

#include <cstdint>
#include <array>

#include <exc.hpp>

namespace gbemu {

	template<std::size_t N>
	using ram_t = std::array<std::uint8_t, N>;

	struct mmu {
		bool bios_accessible { true };

		// bios code 0x0000 -> 0x00FF
		ram_t<0x0100> bios;
		// first rom bank 0x0000 -> 0x3FFF
		ram_t<0x4000> rom0;
		// second rom bank 0x4000 -> 0x7FFF
		ram_t<0x4000> rom1;
		// gpu vram 0x8000 -> 0x9FFF
		ram_t<0x2000> vram;
		// cardrige external memory 0xA000 -> 0xBFFF
		ram_t<0x2000> eram;
		// working ram 0xC000 -> 0xDFFF
		ram_t<0x2000> wram;
		// echo ram (addressed by code) 0xE000 -> 0xFDFF
		// sprite ram
		ram_t<0x00A0> sram;
		// mmio
		ram_t<0x0080> mmio;
		// zero-page ram
		ram_t<0x0080> zram;

		std::uint8_t read_u8(std::uint16_t addr)
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

		void write_u8(std::uint16_t addr, std::uint8_t val)
		{
			switch (addr & 0xF000) {
				// first rom bank | bios
			case 0x0000:				
			case 0x1000:
			case 0x2000:
			case 0x3000:
				throw gbemu_exception{ "Read only memory!" };

				// second rom bank
			case 0x4000:
			case 0x5000:
			case 0x6000:
			case 0x7000:
				throw gbemu_exception{ "Read only memory!" };

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
				else if (addr < 0xFF80)
					mmio[addr - 0xFF00] = val;
				// zram
				else
					zram[addr - 0xFF80] = val;
				break;
			default:
				throw gbemu_exception{ "Invalid address!" };
			}
		}

		std::uint16_t read_u16(std::uint16_t addr) {
			return read_u8(addr) + (read_u8(addr + 1) << 8);
		}

		void write_u16(std::uint16_t addr, std::uint16_t val) {
			write_u8(addr, (val & 0xFF00) >> 8);
			write_u8(addr+1, val & 0x00FF);
		}
	};
}