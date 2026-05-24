#pragma once

#include <cstdint>

#include <alu.h>
#include <card.h>
#include <mmu.h>
#include <ppu.h>
#include <registers.h>

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
        // HALT bug: set when HALT is executed with IME=0 and IF&IE&0x1F != 0.
        // The next fetch must read PC without incrementing it, so the byte
        // after HALT gets executed twice.
        bool halt_bug{false};
        std::uint8_t extra_cycles{0};

        // execute an instruction and returns total cycles
        std::uint8_t step();
        // push u16 on the stack
        void push(std::uint16_t u16);
        // pop u16 from the stack
        std::uint16_t pop_u16();

        // helpers per M-cycle
        uint8_t bus_read(uint16_t addr) { return mmu.read_u8(addr); }
        void bus_write(uint16_t addr, uint8_t v) { mmu.write_u8(addr, v); }
        uint16_t bus_read_u16(uint16_t addr) { return mmu.read_u16(addr); }
        void bus_write_u16(uint16_t a, uint16_t v) { mmu.write_u16(a, v); }

    private:
        // fetch next op
        std::uint16_t fetch();
    };
} // namespace gbemu