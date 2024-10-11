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
	};

	struct cpu {
		registers regs;
		mmu mmu;
		std::optional<cartridge> crd;
		bool interrupt_enabled{ true };

		// fetch next op
		std::uint16_t fetch();
		// decode an opcode into an instruction
		instruction& decode(std::uint16_t op);
		// execute the instruction
		void execute(instruction op);
		// load a cartridge
		void load(cartridge& c);
		// execute a cycle
		void tick();
	};
}