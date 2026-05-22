#pragma once
#include <cpu.h>
#include <unordered_map>

#define DEF_INSTR( o, x, y ) \
	struct x##: instruction { \
		## x ##() : instruction(y) { instruction_set.insert({o, *this}); } \
		void execute(cpu& cpu) override; };

#define INST_INSTR(x) extern gbemu::instruction_types::##x x##_;

namespace gbemu {

	extern std::unordered_map<std::uint16_t, instruction&> instruction_set;
	
	namespace instruction_types {
		DEF_INSTR(0x0000, nop, "NOP");
		DEF_INSTR(0x0031, ldspn16, "LD SP, d16");
		DEF_INSTR(0x00C3, jpa16, "JP a16");
		DEF_INSTR(0x00EA, lda16a, "LD (a16), A");
		DEF_INSTR(0x00F3, di, "DI");
	}

	namespace instructions {
		// 00
		INST_INSTR(nop);
		// 31
		INST_INSTR(ldspn16);
		// C3
		INST_INSTR(jpa16);
		// EA
		INST_INSTR(lda16a);
		// F3
		INST_INSTR(di);
	}

}