#pragma once

#include <cstdint>
#include <array>

#include <exc.hpp>

#define DEF_HWREG(x,addr) \
	std::uint8_t hwr_##x() const { return read_u8(addr); } \
	void hwr_##x(std::uint8_t v) { write_u8(addr, v); }

#define DEF_HWREG_NP(x,addr) \
	std::uint8_t x() const { return read_u8(addr); } \
	void x(std::uint8_t v) { write_u8(addr, v); }

namespace gbemu {

	template<std::size_t N>
	using ram_t = std::array<std::uint8_t, N>;

	struct mmu {
		bool bios_accessible { false };

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

		std::uint8_t read_u8(std::uint16_t addr) const;
		std::int8_t read_i8(std::uint16_t addr) const;
		void write_u8(std::uint16_t addr, std::uint8_t val);
		
		std::uint16_t read_u16(std::uint16_t addr) {
			return read_u8(addr) + (read_u8(addr + 1) << 8);
		}

		void write_u16(std::uint16_t addr, std::uint16_t val) {
			write_u8(addr+1, (val & 0xFF00) >> 8);
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
		DEF_HWREG(stat, 0xFF41);
		DEF_HWREG(scy, 0xFF42);
		DEF_HWREG(scx, 0xFF43);
		DEF_HWREG(ly, 0xFF44);
		DEF_HWREG(lyc, 0xFF45)

		// boot sequence
		void initialize_registers();

		// minimal PPU/timing stub: advances LY (0xFF44) so VBlank-wait
		// polling loops in test ROMs eventually exit. Called once per
		// CPU tick; LY rolls 0..153 every ppu_stub_step instructions.
		void tick_io_stub();

	private:
		unsigned ppu_stub_counter { 0 };
		static constexpr unsigned ppu_stub_step { 32 };
	};
}