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
        // Signal from conditional-branch bodies: set to the (cycles_taken -
        // cycles) delta when the branch is taken. cpu::step() uses it to
        // top up the trailing tick so the total cycles charged match the
        // taken-path count from the dispatch table.
        std::uint8_t extra_cycles{0};

        // Tick callback: installed by core::core() to advance PPU / APU /
        // timer / total_cycles for every M-cycle the CPU consumes (bus
        // accesses + trailing internal cycles). The C-style fn ptr keeps
        // the hot path branch-predictor friendly and avoids the heap-cap
        // overhead of std::function. tick() is a no-op until the callback
        // is wired up, so a free-standing cpu (no core) still works for
        // unit tests.
        using tick_fn_t = void (*)(void* ctx, std::uint8_t t_cycles);
        tick_fn_t tick_fn{nullptr};
        void* tick_ctx{nullptr};

        // Cycles consumed since the most recent step() reset. Bumped by
        // tick(); read by step() to decide the trailing-tick top-up.
        std::uint32_t step_cycles{0};

        void tick(std::uint8_t t_cycles) {
            step_cycles += t_cycles;
            if (tick_fn)
                tick_fn(tick_ctx, t_cycles);
        }

        // execute an instruction and returns total cycles
        std::uint8_t step();
        // push u16 on the stack
        void push(std::uint16_t u16);
        // pop u16 from the stack
        std::uint16_t pop_u16();

        // Bus accessors: tick(4) per byte transferred so PPU / APU / timer
        // see memory accesses at M-cycle granularity. mmu.read_u8 /
        // write_u8 stay untouched so non-CPU consumers (PPU OAM scan, APU
        // wave RAM, debugger peeks) don't trigger spurious ticks.
        std::uint8_t bus_read(std::uint16_t addr) {
            tick(4);
            return mmu.read_u8(addr);
        }
        void bus_write(std::uint16_t addr, std::uint8_t v) {
            tick(4);
            mmu.write_u8(addr, v);
        }
        std::uint16_t bus_read_u16(std::uint16_t addr) {
            std::uint8_t lo = bus_read(addr);
            std::uint8_t hi = bus_read(static_cast<std::uint16_t>(addr + 1));
            return static_cast<std::uint16_t>((hi << 8) | lo);
        }
        void bus_write_u16(std::uint16_t addr, std::uint16_t v) {
            bus_write(addr, static_cast<std::uint8_t>(v & 0xFF));
            bus_write(static_cast<std::uint16_t>(addr + 1), static_cast<std::uint8_t>((v >> 8) & 0xFF));
        }
        std::int8_t bus_read_i8(std::uint16_t addr) { return static_cast<std::int8_t>(bus_read(addr)); }

    private:
        // fetch next op
        std::uint16_t fetch();
    };
} // namespace gbemu