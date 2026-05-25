#include <cpu.h>
#include <exc.hpp>
#include <log.h>
#include <mmu.h>
#include <opcodes.hpp>

using namespace gbemu;

std::uint16_t cpu::fetch() {
    constexpr std::uint8_t prefix = 0xCB;

    // Read first byte through bus_read so the fetch consumes its 4 T-cycles
    // worth of bus ticks. HALT bug: the byte right after HALT is fetched
    // without advancing PC, so the next instruction sees the same byte (and
    // its immediates) again. Only the very first fetch after HALT is
    // affected; the CB-prefix second byte is part of the same instruction
    // and still consumes a normal byte.
    std::uint16_t op = bus_read(regs.pc++);
    if (halt_bug) {
        --regs.pc;
        halt_bug = false;
    }
    if (prefix == op) {
        op <<= 8;
        op |= bus_read(regs.pc++);
    }
    return op;
}

uint8_t cpu::step() {
    step_cycles = 0;

    if (halted || stopped) {
        auto pending = mmu.hwr_if() & mmu.hwr_ie() & 0x1F;
        if (pending) {
            halted = false;
            stopped = false;
        }
        // Halt/stop drains 1 M-cycle per call so the PPU/APU/timer keep
        // ticking and can produce the IRQ that wakes the CPU.
        tick(4);
        return static_cast<std::uint8_t>(step_cycles);
    }

    // EI delay: applica IME=true se EI eseguito allo step precedente
    if (ime_pending && !ei_just_executed) {
        interrupt_enabled = true;
        ime_pending = false;
    }
    ei_just_executed = false;
    extra_cycles = 0;

    auto op = fetch();
    auto& e = ((op & 0xFF00) == 0xCB00) ? dispatch_cb[op & 0xFF] : dispatch_main[op & 0xFF];
    if (!e.fn) {
        // The Sharp LR35902 has 11 truly illegal opcodes (D3 DB DD E3 E4
        // EB EC ED F4 FC FD).  On real hardware they lock the CPU
        // permanently at the current PC — the chip stops fetching and
        // sits there until reset.  Rather than crashing the emulator we
        // emulate the lockup: park PC on the illegal byte, mark halted,
        // log once, drain a single M-cycle.  The halted branch above
        // keeps the PPU/APU/timer running so the user can pause from
        // the debugger and inspect.  All other null-fn cases (CB-prefix
        // holes, opcodes we haven't written a body for yet) still throw
        // because they indicate emulator gaps, not real hardware
        // behaviour the ROM is allowed to rely on.
        const std::uint8_t low = static_cast<std::uint8_t>(op & 0xFF);
        const bool non_cb = (op & 0xFF00) == 0;
        const bool is_illegal =
            non_cb && (low == 0xD3 || low == 0xDB || low == 0xDD || low == 0xE3 || low == 0xE4 || low == 0xEB ||
                       low == 0xEC || low == 0xED || low == 0xF4 || low == 0xFC || low == 0xFD);
        if (is_illegal) {
            // Undo fetch's PC++ so PC sits on the illegal byte, matching
            // the hardware-observable "stuck at this address" behaviour.
            --regs.pc;
            halted = true;
            LOG_ERROR(gbemu::log::root(), "Illegal opcode 0x{:02X} at PC=0x{:04X} — CPU locked", low,
                      static_cast<unsigned>(regs.pc));
            tick(4);
            return static_cast<std::uint8_t>(step_cycles);
        }
        throw gbemu_exception{"Instruction not handled!"};
    }
    e.fn(*this);

    // Hybrid M-cycle accounting: bus accesses ticked during the instruction
    // body; any remaining internal cycles (ALU 16-bit pass, branch PC update,
    // PUSH SP prep, etc.) are bulk-ticked here so the total matches the
    // dispatch-table count. The placement isn't sub-instruction accurate but
    // the per-instruction sum is, which is enough for Blargg cpu_instrs /
    // instr_timing. mem_timing would need per-site internal-tick placement.
    const auto target = static_cast<std::uint32_t>(e.cycles) + extra_cycles;
    if (step_cycles < target) {
        tick(static_cast<std::uint8_t>(target - step_cycles));
    }
    return static_cast<std::uint8_t>(step_cycles);
}

void cpu::push(std::uint16_t u16) {
    // Real hardware pushes high byte at the higher address first, then the
    // low byte at the lower address; each write is one M-cycle so we tick
    // 4 T per byte via bus_write. (The 4 T "internal SP prep" cycle that
    // PUSH rr ordinarily has falls out of the trailing tick in step().)
    regs.sp -= 1;
    bus_write(regs.sp, static_cast<std::uint8_t>((u16 >> 8) & 0xFF));
    regs.sp -= 1;
    bus_write(regs.sp, static_cast<std::uint8_t>(u16 & 0xFF));
}

std::uint16_t cpu::pop_u16() {
    std::uint8_t lo = bus_read(regs.sp);
    regs.sp += 1;
    std::uint8_t hi = bus_read(regs.sp);
    regs.sp += 1;
    return static_cast<std::uint16_t>((hi << 8) | lo);
}
