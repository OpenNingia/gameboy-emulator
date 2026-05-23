#include <cpu.h>
#include <mmu.h>
#include <exc.hpp>
#include <opcodes.hpp>
#include <log.h>

#include <bit>

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
	// mmu.bios_accessible = false;
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

uint8_t cpu::tick() {
	pc_ring[pc_idx] = regs.pc;
	pc_idx = (pc_idx + 1) % pc_ring.size();	
	auto saved_pc = regs.pc;

	if (halted || stopped) {
		auto pending = mmu.hwr_if() & mmu.hwr_ie() & 0x1F;
		if (pending) { 
			halted = false;
			stopped = false;
		}
		ppu.tick(4);
		return 4;
	}	

	// EI delay: applica IME=true se EI eseguito al tick precedente
	if (ime_pending && !ei_just_executed) {
		interrupt_enabled = true;
		ime_pending = false;
	}
	ei_just_executed = false;	

	extra_cycles = 0;

	try {
		auto op = fetch();
		auto& instr = decode(op);
		LOG_TRACE_L1(gbemu::log::root(), "{:04x}  op={:04x}  {}", saved_pc, op, instr.mnemonic);
		instr(*this);

		uint8_t total = instr.cycles + extra_cycles;

		// IRQ dispatch
		auto pending = mmu.hwr_if() & mmu.hwr_ie() & 0x1F;
		if (interrupt_enabled && pending) {
			int b = std::countr_zero(static_cast<unsigned>(pending));
			interrupt_enabled = false;
			mmu.hwr_if(mmu.hwr_if() & ~(1 << b));
			push(regs.pc);
			regs.pc = 0x40 + b * 8;
			total += 20;
		}

		ppu.tick(total);
		return total;
	}
	catch (const gbemu_exception& e) {
		LOG_ERROR(gbemu::log::root(),
			"@PC={:04x} AF={:04x} BC={:04x} DE={:04x} HL={:04x} SP={:04x} : {}",
			saved_pc, regs.af.u16, regs.bc.u16, regs.de.u16, regs.hl.u16, regs.sp, e.what());
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
