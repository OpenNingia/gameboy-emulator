#include <cpu.h>
#include <exc.hpp>
#include <log.h>
#include <mmu.h>
#include <opcodes.hpp>

using namespace gbemu;

std::uint16_t cpu::fetch() {
    constexpr std::uint8_t prefix = 0xCB;

    // read first byte from
    std::uint16_t op = mmu.read_u8(regs.pc++);
    // HALT bug: the byte right after HALT is fetched without advancing PC,
    // so the next instruction sees the same byte (and its immediates) again.
    // Only the very first fetch after HALT is affected; the CB-prefix second
    // byte is part of the same instruction and still consumes a normal byte.
    if (halt_bug) {
        --regs.pc;
        halt_bug = false;
    }
    if (prefix == op) {
        op <<= 8;
        op |= mmu.read_u8(regs.pc++);
    }
    return op;
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

    // EI delay: applica IME=true se EI eseguito allo step precedente
    if (ime_pending && !ei_just_executed) {
        interrupt_enabled = true;
        ime_pending = false;
    }
    ei_just_executed = false;
    extra_cycles = 0;

    auto op = fetch();
    auto& e = ((op & 0xFF00) == 0xCB00) ? dispatch_cb[op & 0xFF] : dispatch_main[op & 0xFF];
    if (!e.fn)
        throw gbemu_exception{"Instruction not handled!"};
    e.fn(*this);

    return e.cycles + extra_cycles;
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
