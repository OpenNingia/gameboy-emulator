#pragma once
#ifndef _H_ALU_H_
#define _H_ALU_H_

#include <registers.h>

namespace gbemu {
	struct alu
	{
		alu(registers& r) : regs(r) { }

		std::uint8_t add(std::uint8_t a, std::uint8_t b) {
			auto r = static_cast<std::uint16_t>(a) + b;
			regs.c_flag(r > 0xFF);
			// set z_flag, n_flag, h_flag
			regs.z_flag(static_cast<std::uint8_t>(r) == 0);
			regs.n_flag(false);
			regs.h_flag((a & 0xF) + (b & 0xF) > 0xF);
			return static_cast<std::uint8_t>(r);
		}

		std::uint16_t add_hl(std::uint16_t a, std::uint16_t b) {
			auto r = static_cast<std::uint32_t>(a) + b;
			regs.c_flag(r > 0xFFFF);
			regs.n_flag(false);
			regs.h_flag((a & 0xFFF) + (b & 0xFFF) > 0xFFF);
			return static_cast<std::uint16_t>(r);
		}		

		std::uint16_t sub(std::uint16_t a, std::uint16_t b) {
			auto r = static_cast<std::uint32_t>(a) - b;
			regs.c_flag(a < b);
			// set z_flag, n_flag, h_flag
			regs.z_flag(static_cast<std::uint16_t>(r) == 0);
			regs.n_flag(true);
			regs.h_flag((a & 0xFFF) < (b & 0xFFF));
			return static_cast<std::uint16_t>(r);
		}

		std::uint8_t sub(std::uint8_t a, std::uint8_t b) {
			auto r = static_cast<std::uint16_t>(a) - b;
			regs.c_flag(a < b);
			// set z_flag, n_flag, h_flag
			regs.z_flag(static_cast<std::uint8_t>(r) == 0);
			regs.n_flag(true);
			regs.h_flag((a & 0xF) < (b & 0xF));
			return static_cast<std::uint8_t>(r);
		}

		void cmp(std::uint8_t a, std::uint8_t b) {
			sub(a, b);
		}

		std::uint8_t inc(std::uint8_t a) {
			auto r = a + 1;
			regs.z_flag(static_cast<std::uint8_t>(r) == 0);
			regs.n_flag(false);
			regs.h_flag((a & 0xF) == 0xF);
			return r;
		}

		std::uint8_t dec(std::uint8_t a) {
			auto r = a - 1;
			regs.z_flag(static_cast<std::uint8_t>(r) == 0);
			regs.n_flag(true);
			regs.h_flag((a & 0xF) == 0x0);
			return r;
		}		

		std::uint8_t rl(std::uint8_t a) {
			std::uint8_t old_carry = regs.c_flag() ? 1 : 0;
			std::uint8_t r = static_cast<std::uint8_t>((a << 1) | old_carry);
			regs.z_flag(r == 0);
			regs.n_flag(false);
			regs.h_flag(false);
			regs.c_flag((a & 0x80) != 0);
			return r;
		}

		std::uint8_t or_(std::uint8_t a, std::uint8_t b) {
			auto r = a |= b;
			regs.z_flag(r == 0);
			regs.n_flag(false);
			regs.h_flag(false);
			regs.c_flag(false);
			return r;
		}

		std::uint8_t adc(std::uint8_t a, std::uint8_t b) {
			auto cy = regs.c_flag() ? 1 : 0;
			auto r = static_cast<std::uint16_t>(a) + b + cy;
			regs.c_flag(r > 0xFF);
			// set z_flag, n_flag, h_flag
			regs.z_flag(static_cast<std::uint8_t>(r) == 0);
			regs.n_flag(false);
			regs.h_flag((a & 0xF) + (b & 0xF) + cy > 0xF);
			return static_cast<std::uint8_t>(r);
		}

		std::uint8_t sbc(std::uint8_t a, std::uint8_t b) {
			auto cy = regs.c_flag() ? 1 : 0;
			auto r = static_cast<std::uint16_t>(a) - b - cy;
			regs.c_flag(a < b + cy);
			// set z_flag, n_flag, h_flag
			regs.z_flag(static_cast<std::uint8_t>(r) == 0);
			regs.n_flag(true);
			regs.h_flag((a & 0xF) < (b & 0xF) + cy);
			return static_cast<std::uint8_t>(r);
		}

		std::uint8_t xor_(std::uint8_t a, std::uint8_t b) {
			auto r = a ^= b;
			regs.z_flag(r == 0);
			regs.n_flag(false);
			regs.h_flag(false);
			regs.c_flag(false);
			return r;
		}
	
	private:
		registers& regs;
	};
}
#endif // _H_ALU_H_