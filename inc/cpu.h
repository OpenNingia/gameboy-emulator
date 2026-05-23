#pragma once

#include <cstdint>
#include <string>
#include <optional>

#include <instruction.h>
#include <registers.h>
#include <alu.h>
#include <ppu.h>
#include <mmu.h>
#include <card.h>

namespace gbemu {
	struct cpu {
		cpu(mmu& m, registers& r) : mmu(m), regs(r), alu(r) {}

		registers& regs;
		mmu& mmu;
		alu alu;

		bool interrupt_enabled{false};
		bool stopped{false};
		bool halted{false};
		bool ime_pending{false};
		bool ei_just_executed{false};
		uint8_t extra_cycles{0};

		// execute an instruction and returns total cycles
		uint8_t step();
		// push u16 on the stack
		void push(std::uint16_t u16);
        // pop u16 from the stack
		std::uint16_t pop_u16();			
		// execute the instruction
		void execute(instruction op);

		private:
			// fetch next op
			std::uint16_t fetch();
			// decode an opcode into an instruction
			instruction& decode(std::uint16_t op);
	};
}