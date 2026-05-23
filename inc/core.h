#pragma once
#ifndef _H_CORE_H_
#define _H_CORE_H_

#include <cstdint>
#include <string>
#include <optional>

#include <instruction.h>
#include <registers.h>
#include <cpu.h>
#include <alu.h>
#include <irq.h>
#include <ppu.h>
#include <mmu.h>
#include <card.h>

namespace gbemu {

	struct core {

		core() : 
			regs(), mmu(), ppu(mmu), cpu(mmu, regs), irq(cpu, mmu), crd(), pc_ring(), pc_idx(0) {}

        cpu cpu;
		registers regs;
		mmu mmu;
		ppu ppu;
		irq irq;
        
		std::optional<rom_file> crd;

		std::array<std::uint16_t, 256> pc_ring;
		std::size_t pc_idx;

		// load a cartridge
		void load(rom_file& c);
		// load bios
		void load(bios_file const& c);
		// init registers
		void init();
		// execute an emulation step
		void step();     
	};
}

#endif /* _H_CORE_H_ */