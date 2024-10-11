#include <cpu.h>
#include <mmu.h>
#include <exc.hpp>
#include <opc.h>

#include <unordered_map>

using namespace gbemu;

// opcodes
/*
const std::unordered_map<std::uint16_t, gbemu::instruction&> instruction_set = {
	{0x0000, instructions::nop_},
	{0x00C3, instructions::jpa16_},
	{0x00F3, instructions::di_}
};
*/

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
	if (instruction_set.contains(op))
		return instruction_set.find(op)->second;
	throw gbemu_exception{ "Instruction not handled!" };
}
void cpu::load(cartridge& c) {
	crd = std::move(c);
	// load first two banks
	memcpy(mmu.rom0.data(), crd->data.data(), mmu.rom0.size());
	memcpy(mmu.rom1.data(), crd->data.data() + mmu.rom0.size(), mmu.rom1.size());

	// reset registers
	memset(&regs, 0, sizeof(regs));

	// skip bios
	regs.pc = 0x100;
	mmu.bios_accessible = false;
}

void cpu::tick() {
	auto op = fetch();
	auto& instr = decode(op);
	// execute instruction
	instr(*this);
}