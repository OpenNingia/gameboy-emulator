#include <cpu.h>
#include <mmu.h>
#include <exc.hpp>
#include <opcodes.hpp>
#include <log.h>

using namespace gbemu;

std::uint16_t cpu::fetch() {
	constexpr std::uint8_t prefix = 0xCB;

	// read first byte from
	std::uint16_t op = mmu.read_u8(regs.pc++);
	if (prefix == op) {
		op <<= 8;
		op |= mmu.read_u8(regs.pc++);
	}
	return op;
}

instruction& cpu::decode(std::uint16_t op) {
	auto* p = ((op & 0xFF00) == 0xCB00)
                ? instruction_set_cb[op & 0xFF]
                : instruction_set[op & 0xFF];
	if (!p) throw gbemu_exception{ "Instruction not handled!" };
	return *p;
}

uint8_t cpu::step() {

	if (halted || stopped) {
		auto pending = mmu.hwr_if() & mmu.hwr_ie() & 0x1F;
		if (pending) { 
			halted = false;
			stopped = false;
		}
		return 4;
	}	

	// EI delay: applica IME=true se EI eseguito al tick precedente
	if (ime_pending && !ei_just_executed) {
		interrupt_enabled = true;
		ime_pending = false;
	}
	ei_just_executed = false;	
	extra_cycles = 0;

	auto op = fetch();
	auto& instr = decode(op);
	instr(*this);

	return instr.cycles + extra_cycles;	
}

void cpu::push(std::uint16_t u16) {
	regs.sp -= 2;
	mmu.write_u16(regs.sp, u16);
}

std::uint16_t cpu::pop_u16() {
	auto u16 = mmu.read_u16(regs.sp);
	regs.sp += 2;
	return u16;
}
