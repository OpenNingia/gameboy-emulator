#include <opc.h>

#define IMPL_INSTR(x) \
	gbemu::instruction_types::##x gbemu::instructions::x##_{}; \
	void gbemu::instruction_types::##x##::execute(cpu& cpu)

std::unordered_map<std::uint16_t, gbemu::instruction&> gbemu::instruction_set;

// 00 NOP
IMPL_INSTR(nop) { }

// C3 JP_NN
IMPL_INSTR(jpa16) {
	auto nn = cpu.mmu.read_u16(cpu.regs.pc);
	cpu.regs.pc = nn;

	// clock.m(3)
	// clock.t(16)
}

// F3 DI
IMPL_INSTR(di) {
	cpu.interrupt_enabled = false;

	// clock.m(x)
	// clock.t(y)
}

// LD SP n16
IMPL_INSTR(ldspn16) {
	auto nn = cpu.mmu.read_u16(cpu.regs.pc);
	cpu.regs.sp = nn;
	cpu.regs.pc += 2;

	// clock.m(x)
	// clock.t(y)
}

IMPL_INSTR(lda16a) {
	auto nn = cpu.mmu.read_u16(cpu.regs.pc);

}