#pragma once

#include <cstdint>
#include <string>
#include <optional>

#include <mmu.h>
#include <card.h>

namespace gbemu {
	union u16reg {
		std::uint16_t u16;
		struct {
			std::uint8_t lo;
			std::uint8_t hi;
		};
	};

	struct cpu;
	struct instruction {
		instruction(std::string m) : mnemonic(std::move(m)) {}
		//
		std::string mnemonic;
		
		void operator() (cpu& cpu) { execute(cpu); };
	protected:
		virtual void execute(cpu& cpu) = 0;
	};

	struct registers {
		u16reg af;
		u16reg bc;
		u16reg de;
		u16reg hl;

		std::uint16_t sp;
		std::uint16_t pc;		

		// flags
		// zero
		bool z_flag() const { return af.lo & 0x80; }
		void z_flag(bool v) { if (v) af.lo |= 0x80; else af.lo &= 0x7F; }

		// sub carry
		bool n_flag() const { return af.lo & 0x40; }
		void n_flag(bool v) { if (v) af.lo |= 0x40; else af.lo &= 0xBF; }

		// half carry
		bool h_flag() const { return af.lo & 0x20; }
		void h_flag(bool v) { if (v) af.lo |= 0x20; else af.lo &= 0xDF; }

		// carry
		bool c_flag() const { return af.lo & 0x10; }
		void c_flag(bool v) { if (v) af.lo |= 0x10; else af.lo &= 0xEF; }
	};

	struct alu
	{
		alu(registers& r) : regs(r) { }

		std::uint8_t add(std::uint8_t a, std::uint8_t b) {
			regs.c_flag((static_cast<std::uint16_t>(a) + b) > 0xFF);
			// set z_flag, n_flag, h_flag
			regs.z_flag((a + b) == 0);
			regs.n_flag(false);
			regs.h_flag((a & 0xF) + (b & 0xF) > 0xF);
			return a + b;
		}

		std::uint16_t add(std::uint16_t a, std::uint16_t b) {
			regs.c_flag((static_cast<std::uint32_t>(a) + b) > 0xFFFF);
			// set z_flag, n_flag, h_flag
			regs.z_flag((a + b) == 0);
			regs.n_flag(false);
			regs.h_flag((a & 0xFFF) + (b & 0xFFF) > 0xFFF);
			return a + b;
		}

		std::uint16_t sub(std::uint16_t a, std::uint16_t b) {
			regs.c_flag(a < b);
			// set z_flag, n_flag, h_flag
			regs.z_flag((a - b) == 0);
			regs.n_flag(true);
			regs.h_flag((a & 0xFFF) < (b & 0xFFF));
			return a - b;
		}

		std::uint8_t sub(std::uint8_t a, std::uint8_t b) {
			regs.c_flag(a < b);
			// set z_flag, n_flag, h_flag
			regs.z_flag((a - b) == 0);
			regs.n_flag(true);
			regs.h_flag((a & 0xF) < (b & 0xF));
			return a - b;
		}
	
	private:
		registers& regs;
	};

	struct cpu {

		cpu() : regs(), mmu(), alu(regs), crd(), interrupt_enabled(true) {}

		registers regs;
		mmu mmu;
		alu alu;
		std::optional<rom_file> crd;
		bool interrupt_enabled{ true };

		// load a cartridge
		void load(rom_file& c);
		// load bios
		void load(bios_file const& c);
		// init registers
		void init();
		// execute a cycle
		void tick();

		// fetch next op
		std::uint16_t fetch();
		// decode an opcode into an instruction
		instruction& decode(std::uint16_t op);
		// execute the instruction
		void execute(instruction op);
		// push u16 on the stack
		void push(std::uint16_t u16);
		std::uint16_t pop_u16();
	};
}