#include <cpu.h>
#include <mmu.h>
#include <exc.hpp>
#include <opc.h>

#include <unordered_map>

#if DEBUG
#include <iostream>
#endif

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
	auto* p = ((op & 0xFF00) == 0xCB00)
                ? instruction_set_cb[op & 0xFF]
                : instruction_set[op & 0xFF];
	if (!p) throw gbemu_exception{ "Instruction not handled!" };
	return *p;
}

void cpu::load(rom_file& c) {
	crd = std::move(c);
	// load first two banks
	auto bank1_sz = std::min(crd->data.size(), mmu.rom0.size());
	auto bank2_sz = std::min(crd->data.size() - bank1_sz, mmu.rom1.size());
	memcpy(mmu.rom0.data(), crd->data.data(), bank1_sz);
	memcpy(mmu.rom1.data(), crd->data.data() + bank1_sz, bank2_sz);

	// reset registers
	memset(&regs, 0, sizeof(regs));

	// skip bios	
	mmu.bios_accessible = false;
}

void cpu::load(bios_file const& c) {
	
	// load first two banks
	memcpy(mmu.bios.data(), c.data.data(), mmu.bios.size());

	// reset registers
	memset(&regs, 0, sizeof(regs));

	// skip bios	
	mmu.bios_accessible = true;
}

void cpu::init() {
	if (mmu.bios_accessible) {
		regs.af.u16 = 0x0000;
		regs.bc.u16 = 0x0000;
		regs.de.u16 = 0x0000;
		regs.hl.u16 = 0x0000;
		regs.sp = 0xFFFE;
		regs.pc = 0x00;
	}
	else {
		// initial registry values
		mmu.initialize_registers();

		regs.af.u16 = 0x01B0;
		regs.bc.u16 = 0x0013;
		regs.de.u16 = 0x00D8;
		regs.hl.u16 = 0x014D;
		regs.sp = 0xFFFE;
		regs.pc = 0x100;
	}
}

void cpu::tick() {
	pc_ring[pc_idx] = regs.pc;
	pc_idx = (pc_idx + 1) % pc_ring.size();

	auto saved_pc = regs.pc;
	try {
		auto op = fetch();
		auto& instr = decode(op);
		instr(*this);
	}
	catch (const gbemu_exception& e) {
#if DEBUG
		std::cerr << std::hex << "@PC=" << saved_pc
			<< " AF=" << regs.af.u16 << " BC=" << regs.bc.u16
			<< " DE=" << regs.de.u16 << " HL=" << regs.hl.u16
			<< " SP=" << regs.sp << " : " << e.what() << "\n";
#endif
		throw;
	}
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

bool cpu::is_stopped() const {
	return stopped;
}

void cpu::stop() {
	stopped = true;
}

void cpu::reset() {
	stopped = false;
}