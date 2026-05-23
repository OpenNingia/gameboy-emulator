// Generated from ops_db.json
// https://gist.github.com/bberak/ca001281bb8431d2706afd31401e802b

#include <exc.hpp>
#include <opcodes.hpp>

#define IMPL_INSTR(x)                                        \
    gbemu::instruction_types::x gbemu::instructions::x##_{}; \
    void gbemu::instruction_types::x::execute(cpu& cpu)

std::array<gbemu::instruction*, 256> gbemu::instruction_set{};
std::array<gbemu::instruction*, 256> gbemu::instruction_set_cb{};

// 7F LD A, A
/* Load the contents of register A into register A. */
IMPL_INSTR(ld_a_a) { /* nop */
}

// 78 LD A, B
/* Load the contents of register B into register A. */
IMPL_INSTR(ld_a_b) {
    cpu.regs.af.hi = cpu.regs.bc.hi;
}

// 79 LD A, C
/* Load the contents of register C into register A. */
IMPL_INSTR(ld_a_c) {
    cpu.regs.af.hi = cpu.regs.bc.lo;
}

// 7A LD A, D
/* Load the contents of register D into register A. */
IMPL_INSTR(ld_a_d) {
    cpu.regs.af.hi = cpu.regs.de.hi;
}

// 7B LD A, E
/* Load the contents of register E into register A. */
IMPL_INSTR(ld_a_e) {
    cpu.regs.af.hi = cpu.regs.de.lo;
}

// 7C LD A, H
/* Load the contents of register H into register A. */
IMPL_INSTR(ld_a_h) {
    cpu.regs.af.hi = cpu.regs.hl.hi;
}

// 7D LD A, L
/* Load the contents of register L into register A. */
IMPL_INSTR(ld_a_l) {
    cpu.regs.af.hi = cpu.regs.hl.lo;
}

// 47 LD B, A
/* Load the contents of register A into register B. */
IMPL_INSTR(ld_b_a) {
    cpu.regs.bc.hi = cpu.regs.af.hi;
}

// 40 LD B, B
/* Load the contents of register B into register B. */
IMPL_INSTR(ld_b_b) { /* nop */
}

// 41 LD B, C
/* Load the contents of register C into register B. */
IMPL_INSTR(ld_b_c) {
    cpu.regs.bc.hi = cpu.regs.bc.lo;
}

// 42 LD B, D
/* Load the contents of register D into register B. */
IMPL_INSTR(ld_b_d) {
    cpu.regs.bc.hi = cpu.regs.de.hi;
}

// 43 LD B, E
/* Load the contents of register E into register B. */
IMPL_INSTR(ld_b_e) {
    cpu.regs.bc.hi = cpu.regs.de.lo;
}

// 44 LD B, H
/* Load the contents of register H into register B. */
IMPL_INSTR(ld_b_h) {
    cpu.regs.bc.hi = cpu.regs.hl.hi;
}

// 45 LD B, L
/* Load the contents of register L into register B. */
IMPL_INSTR(ld_b_l) {
    cpu.regs.bc.hi = cpu.regs.hl.lo;
}

// 4F LD C, A
/* Load the contents of register A into register C. */
IMPL_INSTR(ld_c_a) {
    cpu.regs.bc.lo = cpu.regs.af.hi;
}

// 48 LD C, B
/* Load the contents of register B into register C. */
IMPL_INSTR(ld_c_b) {
    cpu.regs.bc.lo = cpu.regs.bc.hi;
}

// 49 LD C, C
/* Load the contents of register C into register C. */
IMPL_INSTR(ld_c_c) { /* nop */
}

// 4A LD C, D
/* Load the contents of register D into register C. */
IMPL_INSTR(ld_c_d) {
    cpu.regs.bc.lo = cpu.regs.de.hi;
}

// 4B LD C, E
/* Load the contents of register E into register C. */
IMPL_INSTR(ld_c_e) {
    cpu.regs.bc.lo = cpu.regs.de.lo;
}

// 4C LD C, H
/* Load the contents of register H into register C. */
IMPL_INSTR(ld_c_h) {
    cpu.regs.bc.lo = cpu.regs.hl.hi;
}

// 4D LD C, L
/* Load the contents of register L into register C. */
IMPL_INSTR(ld_c_l) {
    cpu.regs.bc.lo = cpu.regs.hl.lo;
}

// 57 LD D, A
/* Load the contents of register A into register D. */
IMPL_INSTR(ld_d_a) {
    cpu.regs.de.hi = cpu.regs.af.hi;
}

// 50 LD D, B
/* Load the contents of register B into register D. */
IMPL_INSTR(ld_d_b) {
    cpu.regs.de.hi = cpu.regs.bc.hi;
}

// 51 LD D, C
/* Load the contents of register C into register D. */
IMPL_INSTR(ld_d_c) {
    cpu.regs.de.hi = cpu.regs.bc.lo;
}

// 52 LD D, D
/* Load the contents of register D into register D. */
IMPL_INSTR(ld_d_d) { /* nop */
}

// 53 LD D, E
/* Load the contents of register E into register D. */
IMPL_INSTR(ld_d_e) {
    cpu.regs.de.hi = cpu.regs.de.lo;
}

// 54 LD D, H
/* Load the contents of register H into register D. */
IMPL_INSTR(ld_d_h) {
    cpu.regs.de.hi = cpu.regs.hl.hi;
}

// 55 LD D, L
/* Load the contents of register L into register D. */
IMPL_INSTR(ld_d_l) {
    cpu.regs.de.hi = cpu.regs.hl.lo;
}

// 5F LD E, A
/* Load the contents of register A into register E. */
IMPL_INSTR(ld_e_a) {
    cpu.regs.de.lo = cpu.regs.af.hi;
}

// 58 LD E, B
/* Load the contents of register B into register E. */
IMPL_INSTR(ld_e_b) {
    cpu.regs.de.lo = cpu.regs.bc.hi;
}

// 59 LD E, C
/* Load the contents of register C into register E. */
IMPL_INSTR(ld_e_c) {
    cpu.regs.de.lo = cpu.regs.bc.lo;
}

// 5A LD E, D
/* Load the contents of register D into register E. */
IMPL_INSTR(ld_e_d) {
    cpu.regs.de.lo = cpu.regs.de.hi;
}

// 5B LD E, E
/* Load the contents of register E into register E. */
IMPL_INSTR(ld_e_e) { /* nop */
}

// 5C LD E, H
/* Load the contents of register H into register E. */
IMPL_INSTR(ld_e_h) {
    cpu.regs.de.lo = cpu.regs.hl.hi;
}

// 5D LD E, L
/* Load the contents of register L into register E. */
IMPL_INSTR(ld_e_l) {
    cpu.regs.de.lo = cpu.regs.hl.lo;
}

// 67 LD H, A
/* Load the contents of register A into register H. */
IMPL_INSTR(ld_h_a) {
    cpu.regs.hl.hi = cpu.regs.af.hi;
}

// 60 LD H, B
/* Load the contents of register B into register H. */
IMPL_INSTR(ld_h_b) {
    cpu.regs.hl.hi = cpu.regs.bc.hi;
}

// 61 LD H, C
/* Load the contents of register C into register H. */
IMPL_INSTR(ld_h_c) {
    cpu.regs.hl.hi = cpu.regs.bc.lo;
}

// 62 LD H, D
/* Load the contents of register D into register H. */
IMPL_INSTR(ld_h_d) {
    cpu.regs.hl.hi = cpu.regs.de.hi;
}

// 63 LD H, E
/* Load the contents of register E into register H. */
IMPL_INSTR(ld_h_e) {
    cpu.regs.hl.hi = cpu.regs.de.lo;
}

// 64 LD H, H
/* Load the contents of register H into register H. */
IMPL_INSTR(ld_h_h) { /* nop */
}

// 65 LD H, L
/* Load the contents of register L into register H. */
IMPL_INSTR(ld_h_l) {
    cpu.regs.hl.hi = cpu.regs.hl.lo;
}

// 6F LD L, A
/* Load the contents of register A into register L. */
IMPL_INSTR(ld_l_a) {
    cpu.regs.hl.lo = cpu.regs.af.hi;
}

// 68 LD L, B
/* Load the contents of register B into register L. */
IMPL_INSTR(ld_l_b) {
    cpu.regs.hl.lo = cpu.regs.bc.hi;
}

// 69 LD L, C
/* Load the contents of register C into register L. */
IMPL_INSTR(ld_l_c) {
    cpu.regs.hl.lo = cpu.regs.bc.lo;
}

// 6A LD L, D
/* Load the contents of register D into register L. */
IMPL_INSTR(ld_l_d) {
    cpu.regs.hl.lo = cpu.regs.de.hi;
}

// 6B LD L, E
/* Load the contents of register E into register L. */
IMPL_INSTR(ld_l_e) {
    cpu.regs.hl.lo = cpu.regs.de.lo;
}

// 6C LD L, H
/* Load the contents of register H into register L. */
IMPL_INSTR(ld_l_h) {
    cpu.regs.hl.lo = cpu.regs.hl.hi;
}

// 6D LD L, L
/* Load the contents of register L into register L. */
IMPL_INSTR(ld_l_l) { /* nop */
}

// 3E LD A, d8
/* Load the 8-bit immediate operand d8 into register A. */
IMPL_INSTR(ld_a_d8) {
    cpu.regs.af.hi = cpu.mmu.read_u8(cpu.regs.pc++);
}

// 06 LD B, d8
/* Load the 8-bit immediate operand d8 into register B. */
IMPL_INSTR(ld_b_d8) {
    cpu.regs.bc.hi = cpu.mmu.read_u8(cpu.regs.pc++);
}

// 0E LD C, d8
/* Load the 8-bit immediate operand d8 into register C. */
IMPL_INSTR(ld_c_d8) {
    cpu.regs.bc.lo = cpu.mmu.read_u8(cpu.regs.pc++);
}

// 16 LD D, d8
/* Load the 8-bit immediate operand d8 into register D. */
IMPL_INSTR(ld_d_d8) {
    cpu.regs.de.hi = cpu.mmu.read_u8(cpu.regs.pc++);
}

// 1E LD E, d8
/* Load the 8-bit immediate operand d8 into register E. */
IMPL_INSTR(ld_e_d8) {
    cpu.regs.de.lo = cpu.mmu.read_u8(cpu.regs.pc++);
}

// 26 LD H, d8
/* Load the 8-bit immediate operand d8 into register H. */
IMPL_INSTR(ld_h_d8) {
    cpu.regs.hl.hi = cpu.mmu.read_u8(cpu.regs.pc++);
}

// 2E LD L, d8
/* Load the 8-bit immediate operand d8 into register L. */
IMPL_INSTR(ld_l_d8) {
    cpu.regs.hl.lo = cpu.mmu.read_u8(cpu.regs.pc++);
}

// 7E LD A, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register A. */
IMPL_INSTR(ld_a__hl_) {
    cpu.regs.af.hi = cpu.mmu.read_u8(cpu.regs.hl.u16);
}

// 46 LD B, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register B. */
IMPL_INSTR(ld_b__hl_) {
    cpu.regs.bc.hi = cpu.mmu.read_u8(cpu.regs.hl.u16);
}

// 4E LD C, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register C. */
IMPL_INSTR(ld_c__hl_) {
    cpu.regs.bc.lo = cpu.mmu.read_u8(cpu.regs.hl.u16);
}

// 56 LD D, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register D. */
IMPL_INSTR(ld_d__hl_) {
    cpu.regs.de.hi = cpu.mmu.read_u8(cpu.regs.hl.u16);
}

// 5E LD E, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register E. */
IMPL_INSTR(ld_e__hl_) {
    cpu.regs.de.lo = cpu.mmu.read_u8(cpu.regs.hl.u16);
}

// 66 LD H, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register H. */
IMPL_INSTR(ld_h__hl_) {
    cpu.regs.hl.hi = cpu.mmu.read_u8(cpu.regs.hl.u16);
}

// 6E LD L, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register L. */
IMPL_INSTR(ld_l__hl_) {
    cpu.regs.hl.lo = cpu.mmu.read_u8(cpu.regs.hl.u16);
}

// 77 LD (HL), A
/* Store the contents of register A in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__a) {
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.regs.af.hi);
}

// 70 LD (HL), B
/* Store the contents of register B in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__b) {
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.regs.bc.hi);
}

// 71 LD (HL), C
/* Store the contents of register C in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__c) {
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.regs.bc.lo);
}

// 72 LD (HL), D
/* Store the contents of register D in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__d) {
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.regs.de.hi);
}

// 73 LD (HL), E
/* Store the contents of register E in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__e) {
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.regs.de.lo);
}

// 74 LD (HL), H
/* Store the contents of register H in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__h) {
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.regs.hl.hi);
}

// 75 LD (HL), L
/* Store the contents of register L in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__l) {
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.regs.hl.lo);
}

// 36 LD (HL), d8
/* Store the contents of 8-bit immediate operand d8 in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__d8) {
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.mmu.read_u8(cpu.regs.pc++));
}

// 0A LD A, (BC)
/* Load the 8-bit contents of memory specified by register pair BC into register A. */
IMPL_INSTR(ld_a__bc_) {
    cpu.regs.af.hi = cpu.mmu.read_u8(cpu.regs.bc.u16);
}

// 1A LD A, (DE)
/* Load the 8-bit contents of memory specified by register pair DE into register A. */
IMPL_INSTR(ld_a__de_) {
    cpu.regs.af.hi = cpu.mmu.read_u8(cpu.regs.de.u16);
}

// F2 LD A, (C)
/* Load into register A the contents of the internal RAM, port register, or mode register at the address in the range
0xFF00-0xFFFF specified by register C. 0xFF00-0xFF7F: Port/Mode registers, control register, sound register
0xFF80-0xFFFE: Working & Stack RAM (127 bytes)
0xFFFF: Interrupt Enable Register */
IMPL_INSTR(ld_a__c_) {
    cpu.regs.af.hi = cpu.mmu.read_u8(0xFF00 + cpu.regs.bc.lo);
}

// E2 LD (C), A
/* Store the contents of register A in the internal RAM, port register, or mode register at the address in the range
0xFF00-0xFFFF specified by register C. 0xFF00-0xFF7F: Port/Mode registers, control register, sound register
0xFF80-0xFFFE: Working & Stack RAM (127 bytes)
0xFFFF: Interrupt Enable Register */
IMPL_INSTR(ld__c__a) {
    cpu.mmu.write_u8(0xFF00 + cpu.regs.bc.lo, cpu.regs.af.hi);
}

// F0 LD A, (a8)
/* Load into register A the contents of the internal RAM, port register, or mode register at the address in the range
0xFF00-0xFFFF specified by the 8-bit immediate operand a8. Note: Should specify a 16-bit address in the mnemonic portion
for a8, although the immediate operand only has the lower-order 8 bits. 0xFF00-0xFF7F: Port/Mode registers, control
register, sound register 0xFF80-0xFFFE: Working & Stack RAM (127 bytes) 0xFFFF: Interrupt Enable Register */
IMPL_INSTR(ld_a__a8_) {
    auto addr = cpu.mmu.read_u8(cpu.regs.pc++) | 0xFF00;
    cpu.regs.af.hi = cpu.mmu.read_u8(addr);
}

// E0 LD (a8), A
/* Store the contents of register A in the internal RAM, port register, or mode register at the address in the range
0xFF00-0xFFFF specified by the 8-bit immediate operand a8. Note: Should specify a 16-bit address in the mnemonic portion
for a8, although the immediate operand only has the lower-order 8 bits. 0xFF00-0xFF7F: Port/Mode registers, control
register, sound register 0xFF80-0xFFFE: Working & Stack RAM (127 bytes) 0xFFFF: Interrupt Enable Register */
IMPL_INSTR(ld__a8__a) {
    std::uint16_t addr = cpu.mmu.read_u8(cpu.regs.pc++) | 0xFF00;
    cpu.mmu.write_u8(addr, cpu.regs.af.hi);
}

// FA LD A, (a16)
/* Load into register A the contents of the internal RAM or register specified by the 16-bit immediate operand a16. */
IMPL_INSTR(ld_a__a16_) {
    auto addr = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.af.hi = cpu.mmu.read_u8(addr);
    cpu.regs.pc += 2;
}

// EA LD (a16), A
/* Store the contents of register A in the internal RAM or register specified by the 16-bit immediate operand a16. */
IMPL_INSTR(ld__a16__a) {
    auto addr = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.mmu.write_u8(addr, cpu.regs.af.hi);
    cpu.regs.pc += 2;
}

// 2A LD A, (HL+)
/* Load the contents of memory specified by register pair HL into register A, and simultaneously increment the contents
 * of HL. */
IMPL_INSTR(ld_a__hlp_) {
    cpu.regs.af.hi = cpu.mmu.read_u8(cpu.regs.hl.u16++);
}

// 3A LD A, (HL-)
/* Load the contents of memory specified by register pair HL into register A, and simultaneously decrement the contents
 * of HL. */
IMPL_INSTR(ld_a__hlm_) {
    cpu.regs.af.hi = cpu.mmu.read_u8(cpu.regs.hl.u16--);
}

// 02 LD (BC), A
/* Store the contents of register A in the memory location specified by register pair BC. */
IMPL_INSTR(ld__bc__a) {
    cpu.mmu.write_u8(cpu.regs.bc.u16, cpu.regs.af.hi);
}

// 12 LD (DE), A
/* Store the contents of register A in the memory location specified by register pair DE. */
IMPL_INSTR(ld__de__a) {
    cpu.mmu.write_u8(cpu.regs.de.u16, cpu.regs.af.hi);
}

// 22 LD (HL+), A
/* Store the contents of register A into the memory location specified by register pair HL, and simultaneously increment
 * the contents of HL. */
IMPL_INSTR(ld__hlp__a) {
    cpu.mmu.write_u8(cpu.regs.hl.u16++, cpu.regs.af.hi);
}

// 32 LD (HL-), A
/* Store the contents of register A into the memory location specified by register pair HL, and simultaneously decrement
 * the contents of HL. */
IMPL_INSTR(ld__hlm__a) {
    cpu.mmu.write_u8(cpu.regs.hl.u16--, cpu.regs.af.hi);
}

// 01 LD BC, d16
/* Load the 2 bytes of immediate data into register pair BC.
 The first byte of immediate data is the lower byte (i.e., bits 0-7), and the second byte of immediate data is the
 higher byte (i.e., bits 8-15). */
IMPL_INSTR(ld_bc_d16) {
    cpu.regs.bc.u16 = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
}

// 11 LD DE, d16
/* Load the 2 bytes of immediate data into register pair DE.
 The first byte of immediate data is the lower byte (i.e., bits 0-7), and the second byte of immediate data is the
 higher byte (i.e., bits 8-15). */
IMPL_INSTR(ld_de_d16) {
    cpu.regs.de.u16 = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
}

// 21 LD HL, d16
/* Load the 2 bytes of immediate data into register pair HL.
 The first byte of immediate data is the lower byte (i.e., bits 0-7), and the second byte of immediate data is the
 higher byte (i.e., bits 8-15). */
IMPL_INSTR(ld_hl_d16) {
    cpu.regs.hl.u16 = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
}

// 31 LD SP, d16
/* Load the 2 bytes of immediate data into register pair SP.
 The first byte of immediate data is the lower byte (i.e., bits 0-7), and the second byte of immediate data is the
 higher byte (i.e., bits 8-15). */
IMPL_INSTR(ld_sp_d16) {
    auto nn = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.sp = nn;
    cpu.regs.pc += 2;
}

// F9 LD SP, HL
/* Load the contents of register pair HL into the stack pointer SP. */
IMPL_INSTR(ld_sp_hl) {
    cpu.regs.sp = cpu.regs.hl.u16;
}

// C5 PUSH BC
/* Push the contents of register pair BC onto the memory stack by doing the following:
Subtract 1 from the stack pointer SP, and put the contents of the higher portion of register pair BC on the stack.
Subtract 2 from SP, and put the lower portion of register pair BC on the stack.
Decrement SP by 2. */
IMPL_INSTR(push_bc) {
    cpu.push(cpu.regs.bc.u16);
}

// D5 PUSH DE
/* Push the contents of register pair DE onto the memory stack by doing the following:
Subtract 1 from the stack pointer SP, and put the contents of the higher portion of register pair DE on the stack.
Subtract 2 from SP, and put the lower portion of register pair DE on the stack.
Decrement SP by 2. */
IMPL_INSTR(push_de) {
    cpu.push(cpu.regs.de.u16);
}

// E5 PUSH HL
/* Push the contents of register pair HL onto the memory stack by doing the following:
Subtract 1 from the stack pointer SP, and put the contents of the higher portion of register pair HL on the stack.
Subtract 2 from SP, and put the lower portion of register pair HL on the stack.
Decrement SP by 2. */
IMPL_INSTR(push_hl) {
    cpu.push(cpu.regs.hl.u16);
}

// F5 PUSH AF
/* Push the contents of register pair AF onto the memory stack by doing the following:
Subtract 1 from the stack pointer SP, and put the contents of the higher portion of register pair AF on the stack.
Subtract 2 from SP, and put the lower portion of register pair AF on the stack.
Decrement SP by 2. */
IMPL_INSTR(push_af) {
    cpu.push(cpu.regs.af.u16);
}

// C1 POP BC
/* Pop the contents from the memory stack into register pair into register pair BC by doing the following:
Load the contents of memory specified by stack pointer SP into the lower portion of BC.
Add 1 to SP and load the contents from the new memory location into the upper portion of BC.
By the end, SP should be 2 more than its initial value. */
IMPL_INSTR(pop_bc) {
    cpu.regs.bc.u16 = cpu.pop_u16();
}

// D1 POP DE
/* Pop the contents from the memory stack into register pair into register pair DE by doing the following:
Load the contents of memory specified by stack pointer SP into the lower portion of DE.
Add 1 to SP and load the contents from the new memory location into the upper portion of DE.
By the end, SP should be 2 more than its initial value. */
IMPL_INSTR(pop_de) {
    cpu.regs.de.u16 = cpu.pop_u16();
}

// E1 POP HL
/* Pop the contents from the memory stack into register pair into register pair HL by doing the following:
Load the contents of memory specified by stack pointer SP into the lower portion of HL.
Add 1 to SP and load the contents from the new memory location into the upper portion of HL.
By the end, SP should be 2 more than its initial value. */
IMPL_INSTR(pop_hl) {
    cpu.regs.hl.u16 = cpu.pop_u16();
}

// F1 POP AF
/* Pop the contents from the memory stack into register pair into register pair AF by doing the following:
Load the contents of memory specified by stack pointer SP into the lower portion of AF.
Add 1 to SP and load the contents from the new memory location into the upper portion of AF.
By the end, SP should be 2 more than its initial value. */
IMPL_INSTR(pop_af) {
    cpu.regs.af.u16 = cpu.pop_u16() & 0xFFF0;
}

// F8 LD HL, SP+s8
/* Add the 8-bit signed operand s8 (values -128 to +127) to the stack pointer SP, and store the result in register pair
 * HL. */
IMPL_INSTR(ld_hl_spps8) {
    auto s8 = cpu.mmu.read_i8(cpu.regs.pc++);
    cpu.regs.hl.u16 = cpu.alu.add_sp_s8(cpu.regs.sp, s8);
}

// 08 LD (a16), SP
/* Store the lower byte of stack pointer SP at the address specified by the 16-bit immediate operand a16, and store the
 * upper byte of SP at address a16 + 1. */
IMPL_INSTR(ld__a16__sp) {
    auto a16 = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    cpu.mmu.write_u16(a16, cpu.regs.sp);
}

// 87 ADD A, A
/* Add the contents of register A to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_a) {
    cpu.regs.af.hi = cpu.alu.add(cpu.regs.af.hi, cpu.regs.af.hi);
}

// 80 ADD A, B
/* Add the contents of register B to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_b) {
    cpu.regs.af.hi = cpu.alu.add(cpu.regs.af.hi, cpu.regs.bc.hi);
}

// 81 ADD A, C
/* Add the contents of register C to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_c) {
    cpu.regs.af.hi = cpu.alu.add(cpu.regs.af.hi, cpu.regs.bc.lo);
}

// 82 ADD A, D
/* Add the contents of register D to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_d) {
    cpu.regs.af.hi = cpu.alu.add(cpu.regs.af.hi, cpu.regs.de.hi);
}

// 83 ADD A, E
/* Add the contents of register E to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_e) {
    cpu.regs.af.hi = cpu.alu.add(cpu.regs.af.hi, cpu.regs.de.lo);
}

// 84 ADD A, H
/* Add the contents of register H to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_h) {
    cpu.regs.af.hi = cpu.alu.add(cpu.regs.af.hi, cpu.regs.hl.hi);
}

// 85 ADD A, L
/* Add the contents of register L to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_l) {
    cpu.regs.af.hi = cpu.alu.add(cpu.regs.af.hi, cpu.regs.hl.lo);
}

// C6 ADD A, d8
/* Add the contents of the 8-bit immediate operand d8 to the contents of register A, and store the results in register
 * A. */
IMPL_INSTR(add_a_d8) {
    auto d8 = cpu.mmu.read_u8(cpu.regs.pc++);
    cpu.regs.af.hi = cpu.alu.add(cpu.regs.af.hi, d8);
}

// 86 ADD A, (HL)
/* Add the contents of memory specified by register pair HL to the contents of register A, and store the results in
 * register A. */
IMPL_INSTR(add_a__hl_) {
    auto value = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.regs.af.hi = cpu.alu.add(cpu.regs.af.hi, value);
}

// 8F ADC A, A
/* Add the contents of register A and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_a) {
    cpu.regs.af.hi = cpu.alu.adc(cpu.regs.af.hi, cpu.regs.af.hi);
}

// 88 ADC A, B
/* Add the contents of register B and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_b) {
    cpu.regs.af.hi = cpu.alu.adc(cpu.regs.af.hi, cpu.regs.bc.hi);
}

// 89 ADC A, C
/* Add the contents of register C and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_c) {
    cpu.regs.af.hi = cpu.alu.adc(cpu.regs.af.hi, cpu.regs.bc.lo);
}

// 8A ADC A, D
/* Add the contents of register D and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_d) {
    cpu.regs.af.hi = cpu.alu.adc(cpu.regs.af.hi, cpu.regs.de.hi);
}

// 8B ADC A, E
/* Add the contents of register E and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_e) {
    cpu.regs.af.hi = cpu.alu.adc(cpu.regs.af.hi, cpu.regs.de.lo);
}

// 8C ADC A, H
/* Add the contents of register H and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_h) {
    cpu.regs.af.hi = cpu.alu.adc(cpu.regs.af.hi, cpu.regs.hl.hi);
}

// 8D ADC A, L
/* Add the contents of register L and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_l) {
    cpu.regs.af.hi = cpu.alu.adc(cpu.regs.af.hi, cpu.regs.hl.lo);
}

// CE ADC A, d8
/* Add the contents of the 8-bit immediate operand d8 and the CY flag to the contents of register A, and store the
 * results in register A. */
IMPL_INSTR(adc_a_d8) {
    auto d8 = cpu.mmu.read_u8(cpu.regs.pc++);
    cpu.regs.af.hi = cpu.alu.adc(cpu.regs.af.hi, d8);
}

// 8E ADC A, (HL)
/* Add the contents of memory specified by register pair HL and the CY flag to the contents of register A, and store the
 * results in register A. */
IMPL_INSTR(adc_a__hl_) {
    auto value = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.regs.af.hi = cpu.alu.adc(cpu.regs.af.hi, value);
}

// 97 SUB A
/* Subtract the contents of register A from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_a) {
    cpu.regs.af.hi = cpu.alu.sub(cpu.regs.af.hi, cpu.regs.af.hi);
}

// 90 SUB B
/* Subtract the contents of register B from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_b) {
    cpu.regs.af.hi = cpu.alu.sub(cpu.regs.af.hi, cpu.regs.bc.hi);
}

// 91 SUB C
/* Subtract the contents of register C from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_c) {
    cpu.regs.af.hi = cpu.alu.sub(cpu.regs.af.hi, cpu.regs.bc.lo);
}

// 92 SUB D
/* Subtract the contents of register D from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_d) {
    cpu.regs.af.hi = cpu.alu.sub(cpu.regs.af.hi, cpu.regs.de.hi);
}

// 93 SUB E
/* Subtract the contents of register E from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_e) {
    cpu.regs.af.hi = cpu.alu.sub(cpu.regs.af.hi, cpu.regs.de.lo);
}

// 94 SUB H
/* Subtract the contents of register H from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_h) {
    cpu.regs.af.hi = cpu.alu.sub(cpu.regs.af.hi, cpu.regs.hl.hi);
}

// 95 SUB L
/* Subtract the contents of register L from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_l) {
    cpu.regs.af.hi = cpu.alu.sub(cpu.regs.af.hi, cpu.regs.hl.lo);
}

// D6 SUB d8
/* Subtract the contents of the 8-bit immediate operand d8 from the contents of register A, and store the results in
 * register A. */
IMPL_INSTR(sub_d8) {
    auto d8 = cpu.mmu.read_u8(cpu.regs.pc++);
    cpu.regs.af.hi = cpu.alu.sub(cpu.regs.af.hi, d8);
}

// 96 SUB (HL)
/* Subtract the contents of memory specified by register pair HL from the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(sub__hl_) {
    auto value = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.regs.af.hi = cpu.alu.sub(cpu.regs.af.hi, value);
}

// 9F SBC A, A
/* Subtract the contents of register A and the CY flag from the contents of register A, and store the results in
 * register A. */
IMPL_INSTR(sbc_a_a) {
    cpu.regs.af.hi = cpu.alu.sbc(cpu.regs.af.hi, cpu.regs.af.hi);
}

// 98 SBC A, B
/* Subtract the contents of register B and the CY flag from the contents of register A, and store the results in
 * register A. */
IMPL_INSTR(sbc_a_b) {
    cpu.regs.af.hi = cpu.alu.sbc(cpu.regs.af.hi, cpu.regs.bc.hi);
}

// 99 SBC A, C
/* Subtract the contents of register C and the CY flag from the contents of register A, and store the results in
 * register A. */
IMPL_INSTR(sbc_a_c) {
    cpu.regs.af.hi = cpu.alu.sbc(cpu.regs.af.hi, cpu.regs.bc.lo);
}

// 9A SBC A, D
/* Subtract the contents of register D and the CY flag from the contents of register A, and store the results in
 * register A. */
IMPL_INSTR(sbc_a_d) {
    cpu.regs.af.hi = cpu.alu.sbc(cpu.regs.af.hi, cpu.regs.de.hi);
}

// 9B SBC A, E
/* Subtract the contents of register E and the CY flag from the contents of register A, and store the results in
 * register A. */
IMPL_INSTR(sbc_a_e) {
    cpu.regs.af.hi = cpu.alu.sbc(cpu.regs.af.hi, cpu.regs.de.lo);
}

// 9C SBC A, H
/* Subtract the contents of register H and the CY flag from the contents of register A, and store the results in
 * register A. */
IMPL_INSTR(sbc_a_h) {
    cpu.regs.af.hi = cpu.alu.sbc(cpu.regs.af.hi, cpu.regs.hl.hi);
}

// 9D SBC A, L
/* Subtract the contents of register L and the CY flag from the contents of register A, and store the results in
 * register A. */
IMPL_INSTR(sbc_a_l) {
    cpu.regs.af.hi = cpu.alu.sbc(cpu.regs.af.hi, cpu.regs.hl.lo);
}

// DE SBC A, d8
/* Subtract the contents of the 8-bit immediate operand d8 and the carry flag CY from the contents of register A, and
 * store the results in register A. */
IMPL_INSTR(sbc_a_d8) {
    auto d8 = cpu.mmu.read_u8(cpu.regs.pc++);
    cpu.regs.af.hi = cpu.alu.sbc(cpu.regs.af.hi, d8);
}

// 9E SBC A, (HL)
/* Subtract the contents of memory specified by register pair HL and the carry flag CY from the contents of register A,
 * and store the results in register A. */
IMPL_INSTR(sbc_a__hl_) {
    auto value = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.regs.af.hi = cpu.alu.sbc(cpu.regs.af.hi, value);
}

// A7 AND A
/* Take the logical AND for each bit of the contents of register A and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(and_a) {
    cpu.regs.af.hi = cpu.alu.and_(cpu.regs.af.hi, cpu.regs.af.hi);
}

// A0 AND B
/* Take the logical AND for each bit of the contents of register B and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(and_b) {
    cpu.regs.af.hi = cpu.alu.and_(cpu.regs.af.hi, cpu.regs.bc.hi);
}

// A1 AND C
/* Take the logical AND for each bit of the contents of register C and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(and_c) {
    cpu.regs.af.hi = cpu.alu.and_(cpu.regs.af.hi, cpu.regs.bc.lo);
}

// A2 AND D
/* Take the logical AND for each bit of the contents of register D and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(and_d) {
    cpu.regs.af.hi = cpu.alu.and_(cpu.regs.af.hi, cpu.regs.de.hi);
}

// A3 AND E
/* Take the logical AND for each bit of the contents of register E and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(and_e) {
    cpu.regs.af.hi = cpu.alu.and_(cpu.regs.af.hi, cpu.regs.de.lo);
}

// A4 AND H
/* Take the logical AND for each bit of the contents of register H and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(and_h) {
    cpu.regs.af.hi = cpu.alu.and_(cpu.regs.af.hi, cpu.regs.hl.hi);
}

// A5 AND L
/* Take the logical AND for each bit of the contents of register L and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(and_l) {
    cpu.regs.af.hi = cpu.alu.and_(cpu.regs.af.hi, cpu.regs.hl.lo);
}

// E6 AND d8
/* Take the logical AND for each bit of the contents of 8-bit immediate operand d8 and the contents of register A, and
 * store the results in register A. */
IMPL_INSTR(and_d8) {
    auto d8 = cpu.mmu.read_u8(cpu.regs.pc++);
    cpu.regs.af.hi = cpu.alu.and_(cpu.regs.af.hi, d8);
}

// A6 AND (HL)
/* Take the logical AND for each bit of the contents of memory specified by register pair HL and the contents of
 * register A, and store the results in register A. */
IMPL_INSTR(and__hl_) {
    auto value = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.regs.af.hi = cpu.alu.and_(cpu.regs.af.hi, value);
}

// B7 OR A
/* Take the logical OR for each bit of the contents of register A and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(or_a) {
    cpu.regs.af.hi = cpu.alu.or_(cpu.regs.af.hi, cpu.regs.af.hi);
}

// B0 OR B
/* Take the logical OR for each bit of the contents of register B and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(or_b) {
    cpu.regs.af.hi = cpu.alu.or_(cpu.regs.af.hi, cpu.regs.bc.hi);
}

// B1 OR C
/* Take the logical OR for each bit of the contents of register C and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(or_c) {
    cpu.regs.af.hi = cpu.alu.or_(cpu.regs.af.hi, cpu.regs.bc.lo);
}

// B2 OR D
/* Take the logical OR for each bit of the contents of register D and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(or_d) {
    cpu.regs.af.hi = cpu.alu.or_(cpu.regs.af.hi, cpu.regs.de.hi);
}

// B3 OR E
/* Take the logical OR for each bit of the contents of register E and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(or_e) {
    cpu.regs.af.hi = cpu.alu.or_(cpu.regs.af.hi, cpu.regs.de.lo);
}

// B4 OR H
/* Take the logical OR for each bit of the contents of register H and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(or_h) {
    cpu.regs.af.hi = cpu.alu.or_(cpu.regs.af.hi, cpu.regs.hl.hi);
}

// B5 OR L
/* Take the logical OR for each bit of the contents of register L and the contents of register A, and store the results
 * in register A. */
IMPL_INSTR(or_l) {
    cpu.regs.af.hi = cpu.alu.or_(cpu.regs.af.hi, cpu.regs.hl.lo);
}

// F6 OR d8
/* Take the logical OR for each bit of the contents of the 8-bit immediate operand d8 and the contents of register A,
 * and store the results in register A. */
IMPL_INSTR(or_d8) {
    auto d8 = cpu.mmu.read_u8(cpu.regs.pc++);
    cpu.regs.af.hi = cpu.alu.or_(cpu.regs.af.hi, d8);
}

// B6 OR (HL)
/* Take the logical OR for each bit of the contents of memory specified by register pair HL and the contents of register
 * A, and store the results in register A. */
IMPL_INSTR(or__hl_) {
    auto value = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.regs.af.hi = cpu.alu.or_(cpu.regs.af.hi, value);
}

// AF XOR A
/* Take the logical exclusive-OR for each bit of the contents of register A and the contents of register A, and store
 * the results in register A. */
IMPL_INSTR(xor_a) {
    cpu.regs.af.hi ^= cpu.regs.af.hi;
    cpu.regs.z_flag(true);
    cpu.regs.n_flag(false);
    cpu.regs.h_flag(false);
    cpu.regs.c_flag(false);
}

// A8 XOR B
/* Take the logical exclusive-OR for each bit of the contents of register B and the contents of register A, and store
 * the results in register A. */
IMPL_INSTR(xor_b) {
    cpu.regs.af.hi ^= cpu.regs.bc.hi;
    cpu.regs.z_flag(cpu.regs.af.hi == 0x0);
    cpu.regs.n_flag(false);
    cpu.regs.h_flag(false);
    cpu.regs.c_flag(false);
}

// A9 XOR C
/* Take the logical exclusive-OR for each bit of the contents of register C and the contents of register A, and store
 * the results in register A. */
IMPL_INSTR(xor_c) {
    cpu.regs.af.hi ^= cpu.regs.bc.lo;
    cpu.regs.z_flag(cpu.regs.af.hi == 0x0);
    cpu.regs.n_flag(false);
    cpu.regs.h_flag(false);
    cpu.regs.c_flag(false);
}

// AA XOR D
/* Take the logical exclusive-OR for each bit of the contents of register D and the contents of register A, and store
 * the results in register A. */
IMPL_INSTR(xor_d) {
    cpu.regs.af.hi ^= cpu.regs.de.hi;
    cpu.regs.z_flag(cpu.regs.af.hi == 0x0);
    cpu.regs.n_flag(false);
    cpu.regs.h_flag(false);
    cpu.regs.c_flag(false);
}

// AB XOR E
/* Take the logical exclusive-OR for each bit of the contents of register E and the contents of register A, and store
 * the results in register A. */
IMPL_INSTR(xor_e) {
    cpu.regs.af.hi ^= cpu.regs.de.lo;
    cpu.regs.z_flag(cpu.regs.af.hi == 0x0);
    cpu.regs.n_flag(false);
    cpu.regs.h_flag(false);
    cpu.regs.c_flag(false);
}

// AC XOR H
/* Take the logical exclusive-OR for each bit of the contents of register H and the contents of register A, and store
 * the results in register A. */
IMPL_INSTR(xor_h) {
    cpu.regs.af.hi ^= cpu.regs.hl.hi;
    cpu.regs.z_flag(cpu.regs.af.hi == 0x0);
    cpu.regs.n_flag(false);
    cpu.regs.h_flag(false);
    cpu.regs.c_flag(false);
}

// AD XOR L
/* Take the logical exclusive-OR for each bit of the contents of register L and the contents of register A, and store
 * the results in register A. */
IMPL_INSTR(xor_l) {
    cpu.regs.af.hi ^= cpu.regs.hl.lo;
    cpu.regs.z_flag(cpu.regs.af.hi == 0x0);
    cpu.regs.n_flag(false);
    cpu.regs.h_flag(false);
    cpu.regs.c_flag(false);
}

// EE XOR d8
/* Take the logical exclusive-OR for each bit of the contents of the 8-bit immediate operand d8 and the contents of
 * register A, and store the results in register A. */
IMPL_INSTR(xor_d8) {
    auto d8 = cpu.mmu.read_u8(cpu.regs.pc++);
    cpu.regs.af.hi = cpu.alu.xor_(cpu.regs.af.hi, d8);
}

// AE XOR (HL)
/* Take the logical exclusive-OR for each bit of the contents of memory specified by register pair HL and the contents
 * of register A, and store the results in register A. */
IMPL_INSTR(xor__hl_) {
    auto value = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.regs.af.hi = cpu.alu.xor_(cpu.regs.af.hi, value);
}

// BF CP A
/* Compare the contents of register A and the contents of register A by calculating A - A, and set the Z flag if they
are equal. The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_a) {
    cpu.alu.cmp(cpu.regs.af.hi, cpu.regs.af.hi);
}

// B8 CP B
/* Compare the contents of register B and the contents of register A by calculating A - B, and set the Z flag if they
are equal. The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_b) {
    cpu.alu.cmp(cpu.regs.af.hi, cpu.regs.bc.hi);
}

// B9 CP C
/* Compare the contents of register C and the contents of register A by calculating A - C, and set the Z flag if they
are equal. The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_c) {
    cpu.alu.cmp(cpu.regs.af.hi, cpu.regs.bc.lo);
}

// BA CP D
/* Compare the contents of register D and the contents of register A by calculating A - D, and set the Z flag if they
are equal. The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_d) {
    cpu.alu.cmp(cpu.regs.af.hi, cpu.regs.de.hi);
}

// BB CP E
/* Compare the contents of register E and the contents of register A by calculating A - E, and set the Z flag if they
are equal. The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_e) {
    cpu.alu.cmp(cpu.regs.af.hi, cpu.regs.de.lo);
}

// BC CP H
/* Compare the contents of register H and the contents of register A by calculating A - H, and set the Z flag if they
are equal. The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_h) {
    cpu.alu.cmp(cpu.regs.af.hi, cpu.regs.hl.hi);
}

// BD CP L
/* Compare the contents of register L and the contents of register A by calculating A - L, and set the Z flag if they
are equal. The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_l) {
    cpu.alu.cmp(cpu.regs.af.hi, cpu.regs.hl.lo);
}

// FE CP d8
/* Compare the contents of register A and the contents of the 8-bit immediate operand d8 by calculating A - d8, and set
the Z flag if they are equal. The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_d8) {
    auto nn = cpu.mmu.read_u8(cpu.regs.pc++);
    cpu.alu.sub(cpu.regs.af.hi, nn); // setta Z/N/H/C, butta il risultato
}

// BE CP (HL)
/* Compare the contents of memory specified by register pair HL and the contents of register A by calculating A - (HL),
and set the Z flag if they are equal. The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp__hl_) {
    auto nn = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.alu.sub(cpu.regs.af.hi, nn); // setta Z/N/H/C, butta il risultato
}

// 3C INC A
/* Increment the contents of register A by 1. */
IMPL_INSTR(inc_a) {
    cpu.regs.af.hi = cpu.alu.inc(cpu.regs.af.hi);
}

// 04 INC B
/* Increment the contents of register B by 1. */
IMPL_INSTR(inc_b) {
    cpu.regs.bc.hi = cpu.alu.inc(cpu.regs.bc.hi);
}

// 0C INC C
/* Increment the contents of register C by 1. */
IMPL_INSTR(inc_c) {
    cpu.regs.bc.lo = cpu.alu.inc(cpu.regs.bc.lo);
}

// 14 INC D
/* Increment the contents of register D by 1. */
IMPL_INSTR(inc_d) {
    cpu.regs.de.hi = cpu.alu.inc(cpu.regs.de.hi);
}

// 1C INC E
/* Increment the contents of register E by 1. */
IMPL_INSTR(inc_e) {
    cpu.regs.de.lo = cpu.alu.inc(cpu.regs.de.lo);
}

// 24 INC H
/* Increment the contents of register H by 1. */
IMPL_INSTR(inc_h) {
    cpu.regs.hl.hi = cpu.alu.inc(cpu.regs.hl.hi);
}

// 2C INC L
/* Increment the contents of register L by 1. */
IMPL_INSTR(inc_l) {
    cpu.regs.hl.lo = cpu.alu.inc(cpu.regs.hl.lo);
}

// 34 INC (HL)
/* Increment the contents of memory specified by register pair HL by 1. */
IMPL_INSTR(inc__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    v = cpu.alu.inc(v);
    cpu.mmu.write_u8(cpu.regs.hl.u16, v);
}

// 3D DEC A
/* Decrement the contents of register A by 1. */
IMPL_INSTR(dec_a) {
    cpu.regs.af.hi = cpu.alu.dec(cpu.regs.af.hi);
}

// 05 DEC B
/* Decrement the contents of register B by 1. */
IMPL_INSTR(dec_b) {
    cpu.regs.bc.hi = cpu.alu.dec(cpu.regs.bc.hi);
}

// 0D DEC C
/* Decrement the contents of register C by 1. */
IMPL_INSTR(dec_c) {
    cpu.regs.bc.lo = cpu.alu.dec(cpu.regs.bc.lo);
}

// 15 DEC D
/* Decrement the contents of register D by 1. */
IMPL_INSTR(dec_d) {
    cpu.regs.de.hi = cpu.alu.dec(cpu.regs.de.hi);
}

// 1D DEC E
/* Decrement the contents of register E by 1. */
IMPL_INSTR(dec_e) {
    cpu.regs.de.lo = cpu.alu.dec(cpu.regs.de.lo);
}

// 25 DEC H
/* Decrement the contents of register H by 1. */
IMPL_INSTR(dec_h) {
    cpu.regs.hl.hi = cpu.alu.dec(cpu.regs.hl.hi);
}

// 2D DEC L
/* Decrement the contents of register L by 1. */
IMPL_INSTR(dec_l) {
    cpu.regs.hl.lo = cpu.alu.dec(cpu.regs.hl.lo);
}

// 35 DEC (HL)
/* Decrement the contents of memory specified by register pair HL by 1. */
IMPL_INSTR(dec__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    v = cpu.alu.dec(v);
    cpu.mmu.write_u8(cpu.regs.hl.u16, v);
}

// 09 ADD HL, BC
/* Add the contents of register pair BC to the contents of register pair HL, and store the results in register pair HL.
 */
IMPL_INSTR(add_hl_bc) {
    cpu.regs.hl.u16 = cpu.alu.add_hl(cpu.regs.hl.u16, cpu.regs.bc.u16);
}

// 19 ADD HL, DE
/* Add the contents of register pair DE to the contents of register pair HL, and store the results in register pair HL.
 */
IMPL_INSTR(add_hl_de) {
    cpu.regs.hl.u16 = cpu.alu.add_hl(cpu.regs.hl.u16, cpu.regs.de.u16);
}

// 29 ADD HL, HL
/* Add the contents of register pair HL to the contents of register pair HL, and store the results in register pair HL.
 */
IMPL_INSTR(add_hl_hl) {
    cpu.regs.hl.u16 = cpu.alu.add_hl(cpu.regs.hl.u16, cpu.regs.hl.u16);
}

// 39 ADD HL, SP
/* Add the contents of register pair SP to the contents of register pair HL, and store the results in register pair HL.
 */
IMPL_INSTR(add_hl_sp) {
    cpu.regs.hl.u16 = cpu.alu.add_hl(cpu.regs.hl.u16, cpu.regs.sp);
}

// E8 ADD SP, s8
/* Add the contents of the 8-bit signed (2's complement) immediate operand s8 and the stack pointer SP and store the
 * results in SP. */
IMPL_INSTR(add_sp_s8) {
    auto s8 = cpu.mmu.read_i8(cpu.regs.pc++);
    cpu.regs.sp = cpu.alu.add_sp_s8(cpu.regs.sp, s8);
}

// 03 INC BC
/* Increment the contents of register pair BC by 1. */
IMPL_INSTR(inc_bc) {
    cpu.regs.bc.u16 = cpu.alu.inc(cpu.regs.bc.u16);
}

// 13 INC DE
/* Increment the contents of register pair DE by 1. */
IMPL_INSTR(inc_de) {
    cpu.regs.de.u16 = cpu.alu.inc(cpu.regs.de.u16);
}

// 23 INC HL
/* Increment the contents of register pair HL by 1. */
IMPL_INSTR(inc_hl) {
    cpu.regs.hl.u16 = cpu.alu.inc(cpu.regs.hl.u16);
}

// 33 INC SP
/* Increment the contents of register pair SP by 1. */
IMPL_INSTR(inc_sp) {
    cpu.regs.sp = cpu.alu.inc(cpu.regs.sp);
}

// 0B DEC BC
/* Decrement the contents of register pair BC by 1. */
IMPL_INSTR(dec_bc) {
    cpu.regs.bc.u16 = cpu.alu.dec(cpu.regs.bc.u16);
}

// 1B DEC DE
/* Decrement the contents of register pair DE by 1. */
IMPL_INSTR(dec_de) {
    cpu.regs.de.u16 = cpu.alu.dec(cpu.regs.de.u16);
}

// 2B DEC HL
/* Decrement the contents of register pair HL by 1. */
IMPL_INSTR(dec_hl) {
    cpu.regs.hl.u16 = cpu.alu.dec(cpu.regs.hl.u16);
}

// 3B DEC SP
/* Decrement the contents of register pair SP by 1. */
IMPL_INSTR(dec_sp) {
    cpu.regs.sp = cpu.alu.dec(cpu.regs.sp);
}

// 07 RLCA
/* Rotate the contents of register A to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register A. */
IMPL_INSTR(rlca) {
    cpu.regs.af.hi = cpu.alu.rlca(cpu.regs.af.hi);
}

// 17 RLA
/* Rotate the contents of register A to the left, through the carry (CY) flag. That is, the contents of bit 0 are copied
 * to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is
 * repeated in sequence for the rest of the register. The previous contents of the carry flag are copied to bit 0. */
IMPL_INSTR(rla) {
    cpu.regs.af.hi = cpu.alu.rla(cpu.regs.af.hi);
}

// 0F RRCA
/* Rotate the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy) are copied to bit 5. The same operation is repeated in sequence for the rest of
 * the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register A. */
IMPL_INSTR(rrca) {
    cpu.regs.af.hi = cpu.alu.rrca(cpu.regs.af.hi);
}

// 1F RRA
/* Rotate the contents of register A to the right, through the carry (CY) flag. That is, the contents of bit 7 are
 * copied to bit 6, and the previous contents of bit 6 (before the copy) are copied to bit 5. The same operation is
 * repeated in sequence for the rest of the register. The previous contents of the carry flag are copied to bit 7. */
IMPL_INSTR(rra) {
    cpu.regs.af.hi = cpu.alu.rra(cpu.regs.af.hi);
}

// CB07 RLC A
/* Rotate the contents of register A to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register A. */
IMPL_INSTR(rlc_a) {
    cpu.regs.af.hi = cpu.alu.rlc(cpu.regs.af.hi);
}

// CB00 RLC B
/* Rotate the contents of register B to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register B. */
IMPL_INSTR(rlc_b) {
    cpu.regs.bc.hi = cpu.alu.rlc(cpu.regs.bc.hi);
}

// CB01 RLC C
/* Rotate the contents of register C to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register C. */
IMPL_INSTR(rlc_c) {
    cpu.regs.bc.lo = cpu.alu.rlc(cpu.regs.bc.lo);
}

// CB02 RLC D
/* Rotate the contents of register D to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register D. */
IMPL_INSTR(rlc_d) {
    cpu.regs.de.hi = cpu.alu.rlc(cpu.regs.de.hi);
}

// CB03 RLC E
/* Rotate the contents of register E to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register E. */
IMPL_INSTR(rlc_e) {
    cpu.regs.de.lo = cpu.alu.rlc(cpu.regs.de.lo);
}

// CB04 RLC H
/* Rotate the contents of register H to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register H. */
IMPL_INSTR(rlc_h) {
    cpu.regs.hl.hi = cpu.alu.rlc(cpu.regs.hl.hi);
}

// CB05 RLC L
/* Rotate the contents of register L to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register L. */
IMPL_INSTR(rlc_l) {
    cpu.regs.hl.lo = cpu.alu.rlc(cpu.regs.hl.lo);
}

// CB06 RLC (HL)
/* Rotate the contents of memory specified by register pair HL to the left. That is, the contents of bit 0 are copied to
 * bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is
 * repeated in sequence for the rest of the memory location. The contents of bit 7 are placed in both the CY flag and
 * bit 0 of (HL). */
IMPL_INSTR(rlc__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.alu.rlc(v));
}

// CB17 RL A
/* Rotate the contents of register A to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register A. */
IMPL_INSTR(rl_a) {
    cpu.regs.af.hi = cpu.alu.rl(cpu.regs.af.hi);
}

// CB10 RL B
/* Rotate the contents of register B to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register B. */
IMPL_INSTR(rl_b) {
    cpu.regs.bc.hi = cpu.alu.rl(cpu.regs.bc.hi);
}

// CB11 RL C
/* Rotate the contents of register C to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register C. */
IMPL_INSTR(rl_c) {
    cpu.regs.bc.lo = cpu.alu.rl(cpu.regs.bc.lo);
}

// CB12 RL D
/* Rotate the contents of register D to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register D. */
IMPL_INSTR(rl_d) {
    cpu.regs.de.hi = cpu.alu.rl(cpu.regs.de.hi);
}

// CB13 RL E
/* Rotate the contents of register E to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register E. */
IMPL_INSTR(rl_e) {
    cpu.regs.de.lo = cpu.alu.rl(cpu.regs.de.lo);
}

// CB14 RL H
/* Rotate the contents of register H to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register H. */
IMPL_INSTR(rl_h) {
    cpu.regs.hl.hi = cpu.alu.rl(cpu.regs.hl.hi);
}

// CB15 RL L
/* Rotate the contents of register L to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register L. */
IMPL_INSTR(rl_l) {
    cpu.regs.hl.lo = cpu.alu.rl(cpu.regs.hl.lo);
}

// CB16 RL (HL)
/* Rotate the contents of memory specified by register pair HL to the left, through the carry flag. That is, the
 * contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to
 * bit 2. The same operation is repeated in sequence for the rest of the memory location. The previous contents of the
 * CY flag are copied into bit 0 of (HL). */
IMPL_INSTR(rl__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.alu.rl(v));
}

// CB0F RRC A
/* Rotate the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register A. */
IMPL_INSTR(rrc_a) {
    cpu.regs.af.hi = cpu.alu.rrc(cpu.regs.af.hi);
}

// CB08 RRC B
/* Rotate the contents of register B to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register B. */
IMPL_INSTR(rrc_b) {
    cpu.regs.bc.hi = cpu.alu.rrc(cpu.regs.bc.hi);
}

// CB09 RRC C
/* Rotate the contents of register C to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register C. */
IMPL_INSTR(rrc_c) {
    cpu.regs.bc.lo = cpu.alu.rrc(cpu.regs.bc.lo);
}

// CB0A RRC D
/* Rotate the contents of register D to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register D. */
IMPL_INSTR(rrc_d) {
    cpu.regs.de.hi = cpu.alu.rrc(cpu.regs.de.hi);
}

// CB0B RRC E
/* Rotate the contents of register E to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register E. */
IMPL_INSTR(rrc_e) {
    cpu.regs.de.lo = cpu.alu.rrc(cpu.regs.de.lo);
}

// CB0C RRC H
/* Rotate the contents of register H to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register H. */
IMPL_INSTR(rrc_h) {
    cpu.regs.hl.hi = cpu.alu.rrc(cpu.regs.hl.hi);
}

// CB0D RRC L
/* Rotate the contents of register L to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register L. */
IMPL_INSTR(rrc_l) {
    cpu.regs.hl.lo = cpu.alu.rrc(cpu.regs.hl.lo);
}

// CB0E RRC (HL)
/* Rotate the contents of memory specified by register pair HL to the right. That is, the contents of bit 7 are copied
 * to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is
 * repeated in sequence for the rest of the memory location. The contents of bit 0 are placed in both the CY flag and
 * bit 7 of (HL). */
IMPL_INSTR(rrc__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.alu.rrc(v));
}

// CB1F RR A
/* Rotate the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register A. */
IMPL_INSTR(rr_a) {
    cpu.regs.af.hi = cpu.alu.rr(cpu.regs.af.hi);
}

// CB18 RR B
/* Rotate the contents of register B to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register B. */
IMPL_INSTR(rr_b) {
    cpu.regs.bc.hi = cpu.alu.rr(cpu.regs.bc.hi);
}

// CB19 RR C
/* Rotate the contents of register C to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register C. */
IMPL_INSTR(rr_c) {
    cpu.regs.bc.lo = cpu.alu.rr(cpu.regs.bc.lo);
}

// CB1A RR D
/* Rotate the contents of register D to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register D. */
IMPL_INSTR(rr_d) {
    cpu.regs.de.hi = cpu.alu.rr(cpu.regs.de.hi);
}

// CB1B RR E
/* Rotate the contents of register E to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register E. */
IMPL_INSTR(rr_e) {
    cpu.regs.de.lo = cpu.alu.rr(cpu.regs.de.lo);
}

// CB1C RR H
/* Rotate the contents of register H to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register H. */
IMPL_INSTR(rr_h) {
    cpu.regs.hl.hi = cpu.alu.rr(cpu.regs.hl.hi);
}

// CB1D RR L
/* Rotate the contents of register L to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register L. */
IMPL_INSTR(rr_l) {
    cpu.regs.hl.lo = cpu.alu.rr(cpu.regs.hl.lo);
}

// CB1E RR (HL)
/* Rotate the contents of memory specified by register pair HL to the right, through the carry flag. That is, the
 * contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to
 * bit 5. The same operation is repeated in sequence for the rest of the memory location. The previous contents of the
 * CY flag are copied into bit 7 of (HL). */
IMPL_INSTR(rr__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.alu.rr(v));
}

// CB27 SLA A
/* Shift the contents of register A to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register A is reset to 0. */
IMPL_INSTR(sla_a) {
    cpu.regs.af.hi = cpu.alu.sla(cpu.regs.af.hi);
}

// CB20 SLA B
/* Shift the contents of register B to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register B is reset to 0. */
IMPL_INSTR(sla_b) {
    cpu.regs.bc.hi = cpu.alu.sla(cpu.regs.bc.hi);
}

// CB21 SLA C
/* Shift the contents of register C to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register C is reset to 0. */
IMPL_INSTR(sla_c) {
    cpu.regs.bc.lo = cpu.alu.sla(cpu.regs.bc.lo);
}

// CB22 SLA D
/* Shift the contents of register D to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register D is reset to 0. */
IMPL_INSTR(sla_d) {
    cpu.regs.de.hi = cpu.alu.sla(cpu.regs.de.hi);
}

// CB23 SLA E
/* Shift the contents of register E to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register E is reset to 0. */
IMPL_INSTR(sla_e) {
    cpu.regs.de.lo = cpu.alu.sla(cpu.regs.de.lo);
}

// CB24 SLA H
/* Shift the contents of register H to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register H is reset to 0. */
IMPL_INSTR(sla_h) {
    cpu.regs.hl.hi = cpu.alu.sla(cpu.regs.hl.hi);
}

// CB25 SLA L
/* Shift the contents of register L to the left. That is, the contents of bit 0 are copied to bit 1, and the previous
 * contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register L is reset to 0. */
IMPL_INSTR(sla_l) {
    cpu.regs.hl.lo = cpu.alu.sla(cpu.regs.hl.lo);
}

// CB26 SLA (HL)
/* Shift the contents of memory specified by register pair HL to the left. That is, the contents of bit 0 are copied to
 * bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is
 * repeated in sequence for the rest of the memory location. The contents of bit 7 are copied to the CY flag, and bit 0
 * of (HL) is reset to 0. */
IMPL_INSTR(sla__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.alu.sla(v));
}

// CB2F SRA A
/* Shift the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register A is unchanged. */
IMPL_INSTR(sra_a) {
    cpu.regs.af.hi = cpu.alu.sra(cpu.regs.af.hi);
}

// CB28 SRA B
/* Shift the contents of register B to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register B is unchanged. */
IMPL_INSTR(sra_b) {
    cpu.regs.bc.hi = cpu.alu.sra(cpu.regs.bc.hi);
}

// CB29 SRA C
/* Shift the contents of register C to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register C is unchanged. */
IMPL_INSTR(sra_c) {
    cpu.regs.bc.lo = cpu.alu.sra(cpu.regs.bc.lo);
}

// CB2A SRA D
/* Shift the contents of register D to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register D is unchanged. */
IMPL_INSTR(sra_d) {
    cpu.regs.de.hi = cpu.alu.sra(cpu.regs.de.hi);
}

// CB2B SRA E
/* Shift the contents of register E to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register E is unchanged. */
IMPL_INSTR(sra_e) {
    cpu.regs.de.lo = cpu.alu.sra(cpu.regs.de.lo);
}

// CB2C SRA H
/* Shift the contents of register H to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register H is unchanged. */
IMPL_INSTR(sra_h) {
    cpu.regs.hl.hi = cpu.alu.sra(cpu.regs.hl.hi);
}

// CB2D SRA L
/* Shift the contents of register L to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register L is unchanged. */
IMPL_INSTR(sra_l) {
    cpu.regs.hl.lo = cpu.alu.sra(cpu.regs.hl.lo);
}

// CB2E SRA (HL)
/* Shift the contents of memory specified by register pair HL to the right. That is, the contents of bit 7 are copied to
 * bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is
 * repeated in sequence for the rest of the memory location. The contents of bit 0 are copied to the CY flag, and bit 7
 * of (HL) is unchanged. */
IMPL_INSTR(sra__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.alu.sra(v));
}

// CB3F SRL A
/* Shift the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register A is reset to 0. */
IMPL_INSTR(srl_a) {
    cpu.regs.af.hi = cpu.alu.srl(cpu.regs.af.hi);
}

// CB38 SRL B
/* Shift the contents of register B to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register B is reset to 0. */
IMPL_INSTR(srl_b) {
    cpu.regs.bc.hi = cpu.alu.srl(cpu.regs.bc.hi);
}

// CB39 SRL C
/* Shift the contents of register C to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register C is reset to 0. */
IMPL_INSTR(srl_c) {
    cpu.regs.bc.lo = cpu.alu.srl(cpu.regs.bc.lo);
}

// CB3A SRL D
/* Shift the contents of register D to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register D is reset to 0. */
IMPL_INSTR(srl_d) {
    cpu.regs.de.hi = cpu.alu.srl(cpu.regs.de.hi);
}

// CB3B SRL E
/* Shift the contents of register E to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register E is reset to 0. */
IMPL_INSTR(srl_e) {
    cpu.regs.de.lo = cpu.alu.srl(cpu.regs.de.lo);
}

// CB3C SRL H
/* Shift the contents of register H to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register H is reset to 0. */
IMPL_INSTR(srl_h) {
    cpu.regs.hl.hi = cpu.alu.srl(cpu.regs.hl.hi);
}

// CB3D SRL L
/* Shift the contents of register L to the right. That is, the contents of bit 7 are copied to bit 6, and the previous
 * contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the
 * rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register L is reset to 0. */
IMPL_INSTR(srl_l) {
    cpu.regs.hl.lo = cpu.alu.srl(cpu.regs.hl.lo);
}

// CB3E SRL (HL)
/* Shift the contents of memory specified by register pair HL to the right. That is, the contents of bit 7 are copied to
 * bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is
 * repeated in sequence for the rest of the memory location. The contents of bit 0 are copied to the CY flag, and bit 7
 * of (HL) is reset to 0. */
IMPL_INSTR(srl__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.alu.srl(v));
}

// CB37 SWAP A
/* Shift the contents of the lower-order four bits (0-3) of register A to the higher-order four bits (4-7) of the
 * register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_a) {
    cpu.regs.af.hi = cpu.alu.swap(cpu.regs.af.hi);
}

// CB30 SWAP B
/* Shift the contents of the lower-order four bits (0-3) of register B to the higher-order four bits (4-7) of the
 * register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_b) {
    cpu.regs.bc.hi = cpu.alu.swap(cpu.regs.bc.hi);
}

// CB31 SWAP C
/* Shift the contents of the lower-order four bits (0-3) of register C to the higher-order four bits (4-7) of the
 * register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_c) {
    cpu.regs.bc.lo = cpu.alu.swap(cpu.regs.bc.lo);
}

// CB32 SWAP D
/* Shift the contents of the lower-order four bits (0-3) of register D to the higher-order four bits (4-7) of the
 * register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_d) {
    cpu.regs.de.hi = cpu.alu.swap(cpu.regs.de.hi);
}

// CB33 SWAP E
/* Shift the contents of the lower-order four bits (0-3) of register E to the higher-order four bits (4-7) of the
 * register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_e) {
    cpu.regs.de.lo = cpu.alu.swap(cpu.regs.de.lo);
}

// CB34 SWAP H
/* Shift the contents of the lower-order four bits (0-3) of register H to the higher-order four bits (4-7) of the
 * register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_h) {
    cpu.regs.hl.hi = cpu.alu.swap(cpu.regs.hl.hi);
}

// CB35 SWAP L
/* Shift the contents of the lower-order four bits (0-3) of register L to the higher-order four bits (4-7) of the
 * register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_l) {
    cpu.regs.hl.lo = cpu.alu.swap(cpu.regs.hl.lo);
}

// CB36 SWAP (HL)
/* Shift the contents of the lower-order four bits (0-3) of the contents of memory specified by register pair HL to the
 * higher-order four bits (4-7) of that memory location, and shift the contents of the higher-order four bits to the
 * lower-order four bits. */
IMPL_INSTR(swap__hl_) {
    auto v = cpu.mmu.read_u8(cpu.regs.hl.u16);
    cpu.mmu.write_u8(cpu.regs.hl.u16, cpu.alu.swap(v));
}

// CB47 BIT 0, A
/* Copy the complement of the contents of bit 0 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_a) {
    cpu.alu.bit(cpu.regs.af.hi, 0);
}

// CB40 BIT 0, B
/* Copy the complement of the contents of bit 0 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_b) {
    cpu.alu.bit(cpu.regs.bc.hi, 0);
}

// CB41 BIT 0, C
/* Copy the complement of the contents of bit 0 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_c) {
    cpu.alu.bit(cpu.regs.bc.lo, 0);
}

// CB42 BIT 0, D
/* Copy the complement of the contents of bit 0 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_d) {
    cpu.alu.bit(cpu.regs.de.hi, 0);
}

// CB43 BIT 0, E
/* Copy the complement of the contents of bit 0 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_e) {
    cpu.alu.bit(cpu.regs.de.lo, 0);
}

// CB44 BIT 0, H
/* Copy the complement of the contents of bit 0 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_h) {
    cpu.alu.bit(cpu.regs.hl.hi, 0);
}

// CB45 BIT 0, L
/* Copy the complement of the contents of bit 0 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_l) {
    cpu.alu.bit(cpu.regs.hl.lo, 0);
}

// CB4F BIT 1, A
/* Copy the complement of the contents of bit 1 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_a) {
    cpu.alu.bit(cpu.regs.af.hi, 1);
}

// CB48 BIT 1, B
/* Copy the complement of the contents of bit 1 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_b) {
    cpu.alu.bit(cpu.regs.bc.hi, 1);
}

// CB49 BIT 1, C
/* Copy the complement of the contents of bit 1 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_c) {
    cpu.alu.bit(cpu.regs.bc.lo, 1);
}

// CB4A BIT 1, D
/* Copy the complement of the contents of bit 1 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_d) {
    cpu.alu.bit(cpu.regs.de.hi, 1);
}

// CB4B BIT 1, E
/* Copy the complement of the contents of bit 1 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_e) {
    cpu.alu.bit(cpu.regs.de.lo, 1);
}

// CB4C BIT 1, H
/* Copy the complement of the contents of bit 1 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_h) {
    cpu.alu.bit(cpu.regs.hl.hi, 1);
}

// CB4D BIT 1, L
/* Copy the complement of the contents of bit 1 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_l) {
    cpu.alu.bit(cpu.regs.hl.lo, 1);
}

// CB57 BIT 2, A
/* Copy the complement of the contents of bit 2 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_a) {
    cpu.alu.bit(cpu.regs.af.hi, 2);
}

// CB50 BIT 2, B
/* Copy the complement of the contents of bit 2 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_b) {
    cpu.alu.bit(cpu.regs.bc.hi, 2);
}

// CB51 BIT 2, C
/* Copy the complement of the contents of bit 2 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_c) {
    cpu.alu.bit(cpu.regs.bc.lo, 2);
}

// CB52 BIT 2, D
/* Copy the complement of the contents of bit 2 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_d) {
    cpu.alu.bit(cpu.regs.de.hi, 2);
}

// CB53 BIT 2, E
/* Copy the complement of the contents of bit 2 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_e) {
    cpu.alu.bit(cpu.regs.de.lo, 2);
}

// CB54 BIT 2, H
/* Copy the complement of the contents of bit 2 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_h) {
    cpu.alu.bit(cpu.regs.hl.hi, 2);
}

// CB55 BIT 2, L
/* Copy the complement of the contents of bit 2 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_l) {
    cpu.alu.bit(cpu.regs.hl.lo, 2);
}

// CB5F BIT 3, A
/* Copy the complement of the contents of bit 3 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_a) {
    cpu.alu.bit(cpu.regs.af.hi, 3);
}

// CB58 BIT 3, B
/* Copy the complement of the contents of bit 3 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_b) {
    cpu.alu.bit(cpu.regs.bc.hi, 3);
}

// CB59 BIT 3, C
/* Copy the complement of the contents of bit 3 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_c) {
    cpu.alu.bit(cpu.regs.bc.lo, 3);
}

// CB5A BIT 3, D
/* Copy the complement of the contents of bit 3 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_d) {
    cpu.alu.bit(cpu.regs.de.hi, 3);
}

// CB5B BIT 3, E
/* Copy the complement of the contents of bit 3 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_e) {
    cpu.alu.bit(cpu.regs.de.lo, 3);
}

// CB5C BIT 3, H
/* Copy the complement of the contents of bit 3 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_h) {
    cpu.alu.bit(cpu.regs.hl.hi, 3);
}

// CB5D BIT 3, L
/* Copy the complement of the contents of bit 3 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_l) {
    cpu.alu.bit(cpu.regs.hl.lo, 3);
}

// CB67 BIT 4, A
/* Copy the complement of the contents of bit 4 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_a) {
    cpu.alu.bit(cpu.regs.af.hi, 4);
}

// CB60 BIT 4, B
/* Copy the complement of the contents of bit 4 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_b) {
    cpu.alu.bit(cpu.regs.bc.hi, 4);
}

// CB61 BIT 4, C
/* Copy the complement of the contents of bit 4 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_c) {
    cpu.alu.bit(cpu.regs.bc.lo, 4);
}

// CB62 BIT 4, D
/* Copy the complement of the contents of bit 4 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_d) {
    cpu.alu.bit(cpu.regs.de.hi, 4);
}

// CB63 BIT 4, E
/* Copy the complement of the contents of bit 4 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_e) {
    cpu.alu.bit(cpu.regs.de.lo, 4);
}

// CB64 BIT 4, H
/* Copy the complement of the contents of bit 4 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_h) {
    cpu.alu.bit(cpu.regs.hl.hi, 4);
}

// CB65 BIT 4, L
/* Copy the complement of the contents of bit 4 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_l) {
    cpu.alu.bit(cpu.regs.hl.lo, 4);
}

// CB6F BIT 5, A
/* Copy the complement of the contents of bit 5 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_a) {
    cpu.alu.bit(cpu.regs.af.hi, 5);
}

// CB68 BIT 5, B
/* Copy the complement of the contents of bit 5 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_b) {
    cpu.alu.bit(cpu.regs.bc.hi, 5);
}

// CB69 BIT 5, C
/* Copy the complement of the contents of bit 5 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_c) {
    cpu.alu.bit(cpu.regs.bc.lo, 5);
}

// CB6A BIT 5, D
/* Copy the complement of the contents of bit 5 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_d) {
    cpu.alu.bit(cpu.regs.de.hi, 5);
}

// CB6B BIT 5, E
/* Copy the complement of the contents of bit 5 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_e) {
    cpu.alu.bit(cpu.regs.de.lo, 5);
}

// CB6C BIT 5, H
/* Copy the complement of the contents of bit 5 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_h) {
    cpu.alu.bit(cpu.regs.hl.hi, 5);
}

// CB6D BIT 5, L
/* Copy the complement of the contents of bit 5 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_l) {
    cpu.alu.bit(cpu.regs.hl.lo, 5);
}

// CB77 BIT 6, A
/* Copy the complement of the contents of bit 6 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_a) {
    cpu.alu.bit(cpu.regs.af.hi, 6);
}

// CB70 BIT 6, B
/* Copy the complement of the contents of bit 6 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_b) {
    cpu.alu.bit(cpu.regs.bc.hi, 6);
}

// CB71 BIT 6, C
/* Copy the complement of the contents of bit 6 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_c) {
    cpu.alu.bit(cpu.regs.bc.lo, 6);
}

// CB72 BIT 6, D
/* Copy the complement of the contents of bit 6 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_d) {
    cpu.alu.bit(cpu.regs.de.hi, 6);
}

// CB73 BIT 6, E
/* Copy the complement of the contents of bit 6 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_e) {
    cpu.alu.bit(cpu.regs.de.lo, 6);
}

// CB74 BIT 6, H
/* Copy the complement of the contents of bit 6 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_h) {
    cpu.alu.bit(cpu.regs.hl.hi, 6);
}

// CB75 BIT 6, L
/* Copy the complement of the contents of bit 6 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_l) {
    cpu.alu.bit(cpu.regs.hl.lo, 6);
}

// CB7F BIT 7, A
/* Copy the complement of the contents of bit 7 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_a) {
    cpu.alu.bit(cpu.regs.af.hi, 7);
}

// CB78 BIT 7, B
/* Copy the complement of the contents of bit 7 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_b) {
    cpu.alu.bit(cpu.regs.bc.hi, 7);
}

// CB79 BIT 7, C
/* Copy the complement of the contents of bit 7 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_c) {
    cpu.alu.bit(cpu.regs.bc.lo, 7);
}

// CB7A BIT 7, D
/* Copy the complement of the contents of bit 7 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_d) {
    cpu.alu.bit(cpu.regs.de.hi, 7);
}

// CB7B BIT 7, E
/* Copy the complement of the contents of bit 7 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_e) {
    cpu.alu.bit(cpu.regs.de.lo, 7);
}

// CB7C BIT 7, H
/* Copy the complement of the contents of bit 7 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_h) {
    cpu.alu.bit(cpu.regs.hl.hi, 7);
}

// CB7D BIT 7, L
/* Copy the complement of the contents of bit 7 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_l) {
    cpu.alu.bit(cpu.regs.hl.lo, 7);
}

// CB46 BIT 0, (HL)
/* Copy the complement of the contents of bit 0 in the memory location specified by register pair HL to the Z flag of
 * the program status word (PSW). */
IMPL_INSTR(bit_0__hl_) {
    cpu.alu.bit(cpu.mmu.read_u8(cpu.regs.hl.u16), 0);
}

// CB4E BIT 1, (HL)
/* Copy the complement of the contents of bit 1 in the memory location specified by register pair HL to the Z flag of
 * the program status word (PSW). */
IMPL_INSTR(bit_1__hl_) {
    cpu.alu.bit(cpu.mmu.read_u8(cpu.regs.hl.u16), 1);
}

// CB56 BIT 2, (HL)
/* Copy the complement of the contents of bit 2 in the memory location specified by register pair HL to the Z flag of
 * the program status word (PSW). */
IMPL_INSTR(bit_2__hl_) {
    cpu.alu.bit(cpu.mmu.read_u8(cpu.regs.hl.u16), 2);
}

// CB5E BIT 3, (HL)
/* Copy the complement of the contents of bit 3 in the memory location specified by register pair HL to the Z flag of
 * the program status word (PSW). */
IMPL_INSTR(bit_3__hl_) {
    cpu.alu.bit(cpu.mmu.read_u8(cpu.regs.hl.u16), 3);
}

// CB66 BIT 4, (HL)
/* Copy the complement of the contents of bit 4 in the memory location specified by register pair HL to the Z flag of
 * the program status word (PSW). */
IMPL_INSTR(bit_4__hl_) {
    cpu.alu.bit(cpu.mmu.read_u8(cpu.regs.hl.u16), 4);
}

// CB6E BIT 5, (HL)
/* Copy the complement of the contents of bit 5 in the memory location specified by register pair HL to the Z flag of
 * the program status word (PSW). */
IMPL_INSTR(bit_5__hl_) {
    cpu.alu.bit(cpu.mmu.read_u8(cpu.regs.hl.u16), 5);
}

// CB76 BIT 6, (HL)
/* Copy the complement of the contents of bit 6 in the memory location specified by register pair HL to the Z flag of
 * the program status word (PSW). */
IMPL_INSTR(bit_6__hl_) {
    cpu.alu.bit(cpu.mmu.read_u8(cpu.regs.hl.u16), 6);
}

// CB7E BIT 7, (HL)
/* Copy the complement of the contents of bit 7 in the memory location specified by register pair HL to the Z flag of
 * the program status word (PSW). */
IMPL_INSTR(bit_7__hl_) {
    cpu.alu.bit(cpu.mmu.read_u8(cpu.regs.hl.u16), 7);
}

// CBC7 SET 0, A
/* Set bit 0 in register A to 1. */
IMPL_INSTR(set_0_a) {
    cpu.regs.af.hi = cpu.alu.set(cpu.regs.af.hi, 0);
}

// CBC0 SET 0, B
/* Set bit 0 in register B to 1. */
IMPL_INSTR(set_0_b) {
    cpu.regs.bc.hi = cpu.alu.set(cpu.regs.bc.hi, 0);
}

// CBC1 SET 0, C
/* Set bit 0 in register C to 1. */
IMPL_INSTR(set_0_c) {
    cpu.regs.bc.lo = cpu.alu.set(cpu.regs.bc.lo, 0);
}

// CBC2 SET 0, D
/* Set bit 0 in register D to 1. */
IMPL_INSTR(set_0_d) {
    cpu.regs.de.hi = cpu.alu.set(cpu.regs.de.hi, 0);
}

// CBC3 SET 0, E
/* Set bit 0 in register E to 1. */
IMPL_INSTR(set_0_e) {
    cpu.regs.de.lo = cpu.alu.set(cpu.regs.de.lo, 0);
}

// CBC4 SET 0, H
/* Set bit 0 in register H to 1. */
IMPL_INSTR(set_0_h) {
    cpu.regs.hl.hi = cpu.alu.set(cpu.regs.hl.hi, 0);
}

// CBC5 SET 0, L
/* Set bit 0 in register L to 1. */
IMPL_INSTR(set_0_l) {
    cpu.regs.hl.lo = cpu.alu.set(cpu.regs.hl.lo, 0);
}

// CBCF SET 1, A
/* Set bit 1 in register A to 1. */
IMPL_INSTR(set_1_a) {
    cpu.regs.af.hi = cpu.alu.set(cpu.regs.af.hi, 1);
}

// CBC8 SET 1, B
/* Set bit 1 in register B to 1. */
IMPL_INSTR(set_1_b) {
    cpu.regs.bc.hi = cpu.alu.set(cpu.regs.bc.hi, 1);
}

// CBC9 SET 1, C
/* Set bit 1 in register C to 1. */
IMPL_INSTR(set_1_c) {
    cpu.regs.bc.lo = cpu.alu.set(cpu.regs.bc.lo, 1);
}

// CBCA SET 1, D
/* Set bit 1 in register D to 1. */
IMPL_INSTR(set_1_d) {
    cpu.regs.de.hi = cpu.alu.set(cpu.regs.de.hi, 1);
}

// CBCB SET 1, E
/* Set bit 1 in register E to 1. */
IMPL_INSTR(set_1_e) {
    cpu.regs.de.lo = cpu.alu.set(cpu.regs.de.lo, 1);
}

// CBCC SET 1, H
/* Set bit 1 in register H to 1. */
IMPL_INSTR(set_1_h) {
    cpu.regs.hl.hi = cpu.alu.set(cpu.regs.hl.hi, 1);
}

// CBCD SET 1, L
/* Set bit 1 in register L to 1. */
IMPL_INSTR(set_1_l) {
    cpu.regs.hl.lo = cpu.alu.set(cpu.regs.hl.lo, 1);
}

// CBD7 SET 2, A
/* Set bit 2 in register A to 1. */
IMPL_INSTR(set_2_a) {
    cpu.regs.af.hi = cpu.alu.set(cpu.regs.af.hi, 2);
}

// CBD0 SET 2, B
/* Set bit 2 in register B to 1. */
IMPL_INSTR(set_2_b) {
    cpu.regs.bc.hi = cpu.alu.set(cpu.regs.bc.hi, 2);
}

// CBD1 SET 2, C
/* Set bit 2 in register C to 1. */
IMPL_INSTR(set_2_c) {
    cpu.regs.bc.lo = cpu.alu.set(cpu.regs.bc.lo, 2);
}

// CBD2 SET 2, D
/* Set bit 2 in register D to 1. */
IMPL_INSTR(set_2_d) {
    cpu.regs.de.hi = cpu.alu.set(cpu.regs.de.hi, 2);
}

// CBD3 SET 2, E
/* Set bit 2 in register E to 1. */
IMPL_INSTR(set_2_e) {
    cpu.regs.de.lo = cpu.alu.set(cpu.regs.de.lo, 2);
}

// CBD4 SET 2, H
/* Set bit 2 in register H to 1. */
IMPL_INSTR(set_2_h) {
    cpu.regs.hl.hi = cpu.alu.set(cpu.regs.hl.hi, 2);
}

// CBD5 SET 2, L
/* Set bit 2 in register L to 1. */
IMPL_INSTR(set_2_l) {
    cpu.regs.hl.lo = cpu.alu.set(cpu.regs.hl.lo, 2);
}

// CBDF SET 3, A
/* Set bit 3 in register A to 1. */
IMPL_INSTR(set_3_a) {
    cpu.regs.af.hi = cpu.alu.set(cpu.regs.af.hi, 3);
}

// CBD8 SET 3, B
/* Set bit 3 in register B to 1. */
IMPL_INSTR(set_3_b) {
    cpu.regs.bc.hi = cpu.alu.set(cpu.regs.bc.hi, 3);
}

// CBD9 SET 3, C
/* Set bit 3 in register C to 1. */
IMPL_INSTR(set_3_c) {
    cpu.regs.bc.lo = cpu.alu.set(cpu.regs.bc.lo, 3);
}

// CBDA SET 3, D
/* Set bit 3 in register D to 1. */
IMPL_INSTR(set_3_d) {
    cpu.regs.de.hi = cpu.alu.set(cpu.regs.de.hi, 3);
}

// CBDB SET 3, E
/* Set bit 3 in register E to 1. */
IMPL_INSTR(set_3_e) {
    cpu.regs.de.lo = cpu.alu.set(cpu.regs.de.lo, 3);
}

// CBDC SET 3, H
/* Set bit 3 in register H to 1. */
IMPL_INSTR(set_3_h) {
    cpu.regs.hl.hi = cpu.alu.set(cpu.regs.hl.hi, 3);
}

// CBDD SET 3, L
/* Set bit 3 in register L to 1. */
IMPL_INSTR(set_3_l) {
    cpu.regs.hl.lo = cpu.alu.set(cpu.regs.hl.lo, 3);
}

// CBE7 SET 4, A
/* Set bit 4 in register A to 1. */
IMPL_INSTR(set_4_a) {
    cpu.regs.af.hi = cpu.alu.set(cpu.regs.af.hi, 4);
}

// CBE0 SET 4, B
/* Set bit 4 in register B to 1. */
IMPL_INSTR(set_4_b) {
    cpu.regs.bc.hi = cpu.alu.set(cpu.regs.bc.hi, 4);
}

// CBE1 SET 4, C
/* Set bit 4 in register C to 1. */
IMPL_INSTR(set_4_c) {
    cpu.regs.bc.lo = cpu.alu.set(cpu.regs.bc.lo, 4);
}

// CBE2 SET 4, D
/* Set bit 4 in register D to 1. */
IMPL_INSTR(set_4_d) {
    cpu.regs.de.hi = cpu.alu.set(cpu.regs.de.hi, 4);
}

// CBE3 SET 4, E
/* Set bit 4 in register E to 1. */
IMPL_INSTR(set_4_e) {
    cpu.regs.de.lo = cpu.alu.set(cpu.regs.de.lo, 4);
}

// CBE4 SET 4, H
/* Set bit 4 in register H to 1. */
IMPL_INSTR(set_4_h) {
    cpu.regs.hl.hi = cpu.alu.set(cpu.regs.hl.hi, 4);
}

// CBE5 SET 4, L
/* Set bit 4 in register L to 1. */
IMPL_INSTR(set_4_l) {
    cpu.regs.hl.lo = cpu.alu.set(cpu.regs.hl.lo, 4);
}

// CBEF SET 5, A
/* Set bit 5 in register A to 1. */
IMPL_INSTR(set_5_a) {
    cpu.regs.af.hi = cpu.alu.set(cpu.regs.af.hi, 5);
}

// CBE8 SET 5, B
/* Set bit 5 in register B to 1. */
IMPL_INSTR(set_5_b) {
    cpu.regs.bc.hi = cpu.alu.set(cpu.regs.bc.hi, 5);
}

// CBE9 SET 5, C
/* Set bit 5 in register C to 1. */
IMPL_INSTR(set_5_c) {
    cpu.regs.bc.lo = cpu.alu.set(cpu.regs.bc.lo, 5);
}

// CBEA SET 5, D
/* Set bit 5 in register D to 1. */
IMPL_INSTR(set_5_d) {
    cpu.regs.de.hi = cpu.alu.set(cpu.regs.de.hi, 5);
}

// CBEB SET 5, E
/* Set bit 5 in register E to 1. */
IMPL_INSTR(set_5_e) {
    cpu.regs.de.lo = cpu.alu.set(cpu.regs.de.lo, 5);
}

// CBEC SET 5, H
/* Set bit 5 in register H to 1. */
IMPL_INSTR(set_5_h) {
    cpu.regs.hl.hi = cpu.alu.set(cpu.regs.hl.hi, 5);
}

// CBED SET 5, L
/* Set bit 5 in register L to 1. */
IMPL_INSTR(set_5_l) {
    cpu.regs.hl.lo = cpu.alu.set(cpu.regs.hl.lo, 5);
}

// CBF7 SET 6, A
/* Set bit 6 in register A to 1. */
IMPL_INSTR(set_6_a) {
    cpu.regs.af.hi = cpu.alu.set(cpu.regs.af.hi, 6);
}

// CBF0 SET 6, B
/* Set bit 6 in register B to 1. */
IMPL_INSTR(set_6_b) {
    cpu.regs.bc.hi = cpu.alu.set(cpu.regs.bc.hi, 6);
}

// CBF1 SET 6, C
/* Set bit 6 in register C to 1. */
IMPL_INSTR(set_6_c) {
    cpu.regs.bc.lo = cpu.alu.set(cpu.regs.bc.lo, 6);
}

// CBF2 SET 6, D
/* Set bit 6 in register D to 1. */
IMPL_INSTR(set_6_d) {
    cpu.regs.de.hi = cpu.alu.set(cpu.regs.de.hi, 6);
}

// CBF3 SET 6, E
/* Set bit 6 in register E to 1. */
IMPL_INSTR(set_6_e) {
    cpu.regs.de.lo = cpu.alu.set(cpu.regs.de.lo, 6);
}

// CBF4 SET 6, H
/* Set bit 6 in register H to 1. */
IMPL_INSTR(set_6_h) {
    cpu.regs.hl.hi = cpu.alu.set(cpu.regs.hl.hi, 6);
}

// CBF5 SET 6, L
/* Set bit 6 in register L to 1. */
IMPL_INSTR(set_6_l) {
    cpu.regs.hl.lo = cpu.alu.set(cpu.regs.hl.lo, 6);
}

// CBFF SET 7, A
/* Set bit 7 in register A to 1. */
IMPL_INSTR(set_7_a) {
    cpu.regs.af.hi = cpu.alu.set(cpu.regs.af.hi, 7);
}

// CBF8 SET 7, B
/* Set bit 7 in register B to 1. */
IMPL_INSTR(set_7_b) {
    cpu.regs.bc.hi = cpu.alu.set(cpu.regs.bc.hi, 7);
}

// CBF9 SET 7, C
/* Set bit 7 in register C to 1. */
IMPL_INSTR(set_7_c) {
    cpu.regs.bc.lo = cpu.alu.set(cpu.regs.bc.lo, 7);
}

// CBFA SET 7, D
/* Set bit 7 in register D to 1. */
IMPL_INSTR(set_7_d) {
    cpu.regs.de.hi = cpu.alu.set(cpu.regs.de.hi, 7);
}

// CBFB SET 7, E
/* Set bit 7 in register E to 1. */
IMPL_INSTR(set_7_e) {
    cpu.regs.de.lo = cpu.alu.set(cpu.regs.de.lo, 7);
}

// CBFC SET 7, H
/* Set bit 7 in register H to 1. */
IMPL_INSTR(set_7_h) {
    cpu.regs.hl.hi = cpu.alu.set(cpu.regs.hl.hi, 7);
}

// CBFD SET 7, L
/* Set bit 7 in register L to 1. */
IMPL_INSTR(set_7_l) {
    cpu.regs.hl.lo = cpu.alu.set(cpu.regs.hl.lo, 7);
}

// CBC6 SET 0, (HL)
/* Set bit 0 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_0__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.set(cpu.mmu.read_u8(addr), 0));
}

// CBCE SET 1, (HL)
/* Set bit 1 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_1__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.set(cpu.mmu.read_u8(addr), 1));
}

// CBD6 SET 2, (HL)
/* Set bit 2 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_2__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.set(cpu.mmu.read_u8(addr), 2));
}

// CBDE SET 3, (HL)
/* Set bit 3 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_3__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.set(cpu.mmu.read_u8(addr), 3));
}

// CBE6 SET 4, (HL)
/* Set bit 4 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_4__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.set(cpu.mmu.read_u8(addr), 4));
}

// CBEE SET 5, (HL)
/* Set bit 5 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_5__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.set(cpu.mmu.read_u8(addr), 5));
}

// CBF6 SET 6, (HL)
/* Set bit 6 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_6__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.set(cpu.mmu.read_u8(addr), 6));
}

// CBFE SET 7, (HL)
/* Set bit 7 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_7__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.set(cpu.mmu.read_u8(addr), 7));
}

// CB87 RES 0, A
/* Reset bit 0 in register A to 0. */
IMPL_INSTR(res_0_a) {
    cpu.regs.af.hi = cpu.alu.res(cpu.regs.af.hi, 0);
}

// CB80 RES 0, B
/* Reset bit 0 in register B to 0. */
IMPL_INSTR(res_0_b) {
    cpu.regs.bc.hi = cpu.alu.res(cpu.regs.bc.hi, 0);
}

// CB81 RES 0, C
/* Reset bit 0 in register C to 0. */
IMPL_INSTR(res_0_c) {
    cpu.regs.bc.lo = cpu.alu.res(cpu.regs.bc.lo, 0);
}

// CB82 RES 0, D
/* Reset bit 0 in register D to 0. */
IMPL_INSTR(res_0_d) {
    cpu.regs.de.hi = cpu.alu.res(cpu.regs.de.hi, 0);
}

// CB83 RES 0, E
/* Reset bit 0 in register E to 0. */
IMPL_INSTR(res_0_e) {
    cpu.regs.de.lo = cpu.alu.res(cpu.regs.de.lo, 0);
}

// CB84 RES 0, H
/* Reset bit 0 in register H to 0. */
IMPL_INSTR(res_0_h) {
    cpu.regs.hl.hi = cpu.alu.res(cpu.regs.hl.hi, 0);
}

// CB85 RES 0, L
/* Reset bit 0 in register L to 0. */
IMPL_INSTR(res_0_l) {
    cpu.regs.hl.lo = cpu.alu.res(cpu.regs.hl.lo, 0);
}

// CB8F RES 1, A
/* Reset bit 1 in register A to 0. */
IMPL_INSTR(res_1_a) {
    cpu.regs.af.hi = cpu.alu.res(cpu.regs.af.hi, 1);
}

// CB88 RES 1, B
/* Reset bit 1 in register B to 0. */
IMPL_INSTR(res_1_b) {
    cpu.regs.bc.hi = cpu.alu.res(cpu.regs.bc.hi, 1);
}

// CB89 RES 1, C
/* Reset bit 1 in register C to 0. */
IMPL_INSTR(res_1_c) {
    cpu.regs.bc.lo = cpu.alu.res(cpu.regs.bc.lo, 1);
}

// CB8A RES 1, D
/* Reset bit 1 in register D to 0. */
IMPL_INSTR(res_1_d) {
    cpu.regs.de.hi = cpu.alu.res(cpu.regs.de.hi, 1);
}

// CB8B RES 1, E
/* Reset bit 1 in register E to 0. */
IMPL_INSTR(res_1_e) {
    cpu.regs.de.lo = cpu.alu.res(cpu.regs.de.lo, 1);
}

// CB8C RES 1, H
/* Reset bit 1 in register H to 0. */
IMPL_INSTR(res_1_h) {
    cpu.regs.hl.hi = cpu.alu.res(cpu.regs.hl.hi, 1);
}

// CB8D RES 1, L
/* Reset bit 1 in register L to 0. */
IMPL_INSTR(res_1_l) {
    cpu.regs.hl.lo = cpu.alu.res(cpu.regs.hl.lo, 1);
}

// CB97 RES 2, A
/* Reset bit 2 in register A to 0. */
IMPL_INSTR(res_2_a) {
    cpu.regs.af.hi = cpu.alu.res(cpu.regs.af.hi, 2);
}

// CB90 RES 2, B
/* Reset bit 2 in register B to 0. */
IMPL_INSTR(res_2_b) {
    cpu.regs.bc.hi = cpu.alu.res(cpu.regs.bc.hi, 2);
}

// CB91 RES 2, C
/* Reset bit 2 in register C to 0. */
IMPL_INSTR(res_2_c) {
    cpu.regs.bc.lo = cpu.alu.res(cpu.regs.bc.lo, 2);
}

// CB92 RES 2, D
/* Reset bit 2 in register D to 0. */
IMPL_INSTR(res_2_d) {
    cpu.regs.de.hi = cpu.alu.res(cpu.regs.de.hi, 2);
}

// CB93 RES 2, E
/* Reset bit 2 in register E to 0. */
IMPL_INSTR(res_2_e) {
    cpu.regs.de.lo = cpu.alu.res(cpu.regs.de.lo, 2);
}

// CB94 RES 2, H
/* Reset bit 2 in register H to 0. */
IMPL_INSTR(res_2_h) {
    cpu.regs.hl.hi = cpu.alu.res(cpu.regs.hl.hi, 2);
}

// CB95 RES 2, L
/* Reset bit 2 in register L to 0. */
IMPL_INSTR(res_2_l) {
    cpu.regs.hl.lo = cpu.alu.res(cpu.regs.hl.lo, 2);
}

// CB9F RES 3, A
/* Reset bit 3 in register A to 0. */
IMPL_INSTR(res_3_a) {
    cpu.regs.af.hi = cpu.alu.res(cpu.regs.af.hi, 3);
}

// CB98 RES 3, B
/* Reset bit 3 in register B to 0. */
IMPL_INSTR(res_3_b) {
    cpu.regs.bc.hi = cpu.alu.res(cpu.regs.bc.hi, 3);
}

// CB99 RES 3, C
/* Reset bit 3 in register C to 0. */
IMPL_INSTR(res_3_c) {
    cpu.regs.bc.lo = cpu.alu.res(cpu.regs.bc.lo, 3);
}

// CB9A RES 3, D
/* Reset bit 3 in register D to 0. */
IMPL_INSTR(res_3_d) {
    cpu.regs.de.hi = cpu.alu.res(cpu.regs.de.hi, 3);
}

// CB9B RES 3, E
/* Reset bit 3 in register E to 0. */
IMPL_INSTR(res_3_e) {
    cpu.regs.de.lo = cpu.alu.res(cpu.regs.de.lo, 3);
}

// CB9C RES 3, H
/* Reset bit 3 in register H to 0. */
IMPL_INSTR(res_3_h) {
    cpu.regs.hl.hi = cpu.alu.res(cpu.regs.hl.hi, 3);
}

// CB9D RES 3, L
/* Reset bit 3 in register L to 0. */
IMPL_INSTR(res_3_l) {
    cpu.regs.hl.lo = cpu.alu.res(cpu.regs.hl.lo, 3);
}

// CBA7 RES 4, A
/* Reset bit 4 in register A to 0. */
IMPL_INSTR(res_4_a) {
    cpu.regs.af.hi = cpu.alu.res(cpu.regs.af.hi, 4);
}

// CBA0 RES 4, B
/* Reset bit 4 in register B to 0. */
IMPL_INSTR(res_4_b) {
    cpu.regs.bc.hi = cpu.alu.res(cpu.regs.bc.hi, 4);
}

// CBA1 RES 4, C
/* Reset bit 4 in register C to 0. */
IMPL_INSTR(res_4_c) {
    cpu.regs.bc.lo = cpu.alu.res(cpu.regs.bc.lo, 4);
}

// CBA2 RES 4, D
/* Reset bit 4 in register D to 0. */
IMPL_INSTR(res_4_d) {
    cpu.regs.de.hi = cpu.alu.res(cpu.regs.de.hi, 4);
}

// CBA3 RES 4, E
/* Reset bit 4 in register E to 0. */
IMPL_INSTR(res_4_e) {
    cpu.regs.de.lo = cpu.alu.res(cpu.regs.de.lo, 4);
}

// CBA4 RES 4, H
/* Reset bit 4 in register H to 0. */
IMPL_INSTR(res_4_h) {
    cpu.regs.hl.hi = cpu.alu.res(cpu.regs.hl.hi, 4);
}

// CBA5 RES 4, L
/* Reset bit 4 in register L to 0. */
IMPL_INSTR(res_4_l) {
    cpu.regs.hl.lo = cpu.alu.res(cpu.regs.hl.lo, 4);
}

// CBAF RES 5, A
/* Reset bit 5 in register A to 0. */
IMPL_INSTR(res_5_a) {
    cpu.regs.af.hi = cpu.alu.res(cpu.regs.af.hi, 5);
}

// CBA8 RES 5, B
/* Reset bit 5 in register B to 0. */
IMPL_INSTR(res_5_b) {
    cpu.regs.bc.hi = cpu.alu.res(cpu.regs.bc.hi, 5);
}

// CBA9 RES 5, C
/* Reset bit 5 in register C to 0. */
IMPL_INSTR(res_5_c) {
    cpu.regs.bc.lo = cpu.alu.res(cpu.regs.bc.lo, 5);
}

// CBAA RES 5, D
/* Reset bit 5 in register D to 0. */
IMPL_INSTR(res_5_d) {
    cpu.regs.de.hi = cpu.alu.res(cpu.regs.de.hi, 5);
}

// CBAB RES 5, E
/* Reset bit 5 in register E to 0. */
IMPL_INSTR(res_5_e) {
    cpu.regs.de.lo = cpu.alu.res(cpu.regs.de.lo, 5);
}

// CBAC RES 5, H
/* Reset bit 5 in register H to 0. */
IMPL_INSTR(res_5_h) {
    cpu.regs.hl.hi = cpu.alu.res(cpu.regs.hl.hi, 5);
}

// CBAD RES 5, L
/* Reset bit 5 in register L to 0. */
IMPL_INSTR(res_5_l) {
    cpu.regs.hl.lo = cpu.alu.res(cpu.regs.hl.lo, 5);
}

// CBB7 RES 6, A
/* Reset bit 6 in register A to 0. */
IMPL_INSTR(res_6_a) {
    cpu.regs.af.hi = cpu.alu.res(cpu.regs.af.hi, 6);
}

// CBB0 RES 6, B
/* Reset bit 6 in register B to 0. */
IMPL_INSTR(res_6_b) {
    cpu.regs.bc.hi = cpu.alu.res(cpu.regs.bc.hi, 6);
}

// CBB1 RES 6, C
/* Reset bit 6 in register C to 0. */
IMPL_INSTR(res_6_c) {
    cpu.regs.bc.lo = cpu.alu.res(cpu.regs.bc.lo, 6);
}

// CBB2 RES 6, D
/* Reset bit 6 in register D to 0. */
IMPL_INSTR(res_6_d) {
    cpu.regs.de.hi = cpu.alu.res(cpu.regs.de.hi, 6);
}

// CBB3 RES 6, E
/* Reset bit 6 in register E to 0. */
IMPL_INSTR(res_6_e) {
    cpu.regs.de.lo = cpu.alu.res(cpu.regs.de.lo, 6);
}

// CBB4 RES 6, H
/* Reset bit 6 in register H to 0. */
IMPL_INSTR(res_6_h) {
    cpu.regs.hl.hi = cpu.alu.res(cpu.regs.hl.hi, 6);
}

// CBB5 RES 6, L
/* Reset bit 6 in register L to 0. */
IMPL_INSTR(res_6_l) {
    cpu.regs.hl.lo = cpu.alu.res(cpu.regs.hl.lo, 6);
}

// CBBF RES 7, A
/* Reset bit 7 in register A to 0. */
IMPL_INSTR(res_7_a) {
    cpu.regs.af.hi = cpu.alu.res(cpu.regs.af.hi, 7);
}

// CBB8 RES 7, B
/* Reset bit 7 in register B to 0. */
IMPL_INSTR(res_7_b) {
    cpu.regs.bc.hi = cpu.alu.res(cpu.regs.bc.hi, 7);
}

// CBB9 RES 7, C
/* Reset bit 7 in register C to 0. */
IMPL_INSTR(res_7_c) {
    cpu.regs.bc.lo = cpu.alu.res(cpu.regs.bc.lo, 7);
}

// CBBA RES 7, D
/* Reset bit 7 in register D to 0. */
IMPL_INSTR(res_7_d) {
    cpu.regs.de.hi = cpu.alu.res(cpu.regs.de.hi, 7);
}

// CBBB RES 7, E
/* Reset bit 7 in register E to 0. */
IMPL_INSTR(res_7_e) {
    cpu.regs.de.lo = cpu.alu.res(cpu.regs.de.lo, 7);
}

// CBBC RES 7, H
/* Reset bit 7 in register H to 0. */
IMPL_INSTR(res_7_h) {
    cpu.regs.hl.hi = cpu.alu.res(cpu.regs.hl.hi, 7);
}

// CBBD RES 7, L
/* Reset bit 7 in register L to 0. */
IMPL_INSTR(res_7_l) {
    cpu.regs.hl.lo = cpu.alu.res(cpu.regs.hl.lo, 7);
}

// CB86 RES 0, (HL)
/* Reset bit 0 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_0__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.res(cpu.mmu.read_u8(addr), 0));
}

// CB8E RES 1, (HL)
/* Reset bit 1 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_1__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.res(cpu.mmu.read_u8(addr), 1));
}

// CB96 RES 2, (HL)
/* Reset bit 2 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_2__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.res(cpu.mmu.read_u8(addr), 2));
}

// CB9E RES 3, (HL)
/* Reset bit 3 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_3__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.res(cpu.mmu.read_u8(addr), 3));
}

// CBA6 RES 4, (HL)
/* Reset bit 4 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_4__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.res(cpu.mmu.read_u8(addr), 4));
}

// CBAE RES 5, (HL)
/* Reset bit 5 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_5__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.res(cpu.mmu.read_u8(addr), 5));
}

// CBB6 RES 6, (HL)
/* Reset bit 6 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_6__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.res(cpu.mmu.read_u8(addr), 6));
}

// CBBE RES 7, (HL)
/* Reset bit 7 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_7__hl_) {
    const auto addr = cpu.regs.hl.u16;
    cpu.mmu.write_u8(addr, cpu.alu.res(cpu.mmu.read_u8(addr), 7));
}

// C3 JP a16
/* Load the 16-bit immediate operand a16 into the program counter (PC). a16 specifies the address of the subsequently
executed instruction. The second byte of the object code (immediately following the opcode) corresponds to the
lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to the higher-order byte (bits
8-15). */
IMPL_INSTR(jp_a16) {
    auto nn = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc = nn;
}

// C2 JP NZ, a16
/* Load the 16-bit immediate operand a16 into the program counter PC if the Z flag is 0. If the Z flag is 0, then the
subsequent instruction starts at address a16. If not, the contents of PC are incremented, and the next instruction
following the current JP instruction is executed (as usual). The second byte of the object code (immediately following
the opcode) corresponds to the lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to
the higher-order byte (bits 8-15). */
IMPL_INSTR(jp_nz_a16) {
    auto nn = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    if (!cpu.regs.z_flag()) {
        cpu.regs.pc = nn;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// CA JP Z, a16
/* Load the 16-bit immediate operand a16 into the program counter PC if the Z flag is 1. If the Z flag is 1, then the
subsequent instruction starts at address a16. If not, the contents of PC are incremented, and the next instruction
following the current JP instruction is executed (as usual). The second byte of the object code (immediately following
the opcode) corresponds to the lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to
the higher-order byte (bits 8-15). */
IMPL_INSTR(jp_z_a16) {
    auto nn = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    if (cpu.regs.z_flag()) {
        cpu.regs.pc = nn;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// D2 JP NC, a16
/* Load the 16-bit immediate operand a16 into the program counter PC if the CY flag is 0. If the CY flag is 0, then the
subsequent instruction starts at address a16. If not, the contents of PC are incremented, and the next instruction
following the current JP instruction is executed (as usual). The second byte of the object code (immediately following
the opcode) corresponds to the lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to
the higher-order byte (bits 8-15). */
IMPL_INSTR(jp_nc_a16) {
    auto nn = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    if (!cpu.regs.c_flag()) {
        cpu.regs.pc = nn;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// DA JP C, a16
/* Load the 16-bit immediate operand a16 into the program counter PC if the CY flag is 1. If the CY flag is 1, then the
subsequent instruction starts at address a16. If not, the contents of PC are incremented, and the next instruction
following the current JP instruction is executed (as usual). The second byte of the object code (immediately following
the opcode) corresponds to the lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to
the higher-order byte (bits 8-15). */
IMPL_INSTR(jp_c_a16) {
    auto nn = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    if (cpu.regs.c_flag()) {
        cpu.regs.pc = nn;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// 18 JR s8
/* Jump s8 steps from the current address in the program counter (PC). (Jump relative.) */
IMPL_INSTR(jr_s8) {
    auto s8 = cpu.mmu.read_i8(cpu.regs.pc++);
    cpu.regs.pc += s8;
}

// 20 JR NZ, s8
/* If the Z flag is 0, jump s8 steps from the current address stored in the program counter (PC). If not, the
 * instruction following the current JP instruction is executed (as usual). */
IMPL_INSTR(jr_nz_s8) {
    auto s8 = cpu.mmu.read_i8(cpu.regs.pc++);
    if (!cpu.regs.z_flag()) {
        cpu.regs.pc += s8;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// 28 JR Z, s8
/* If the Z flag is 1, jump s8 steps from the current address stored in the program counter (PC). If not, the
 * instruction following the current JP instruction is executed (as usual). */
IMPL_INSTR(jr_z_s8) {
    auto s8 = cpu.mmu.read_i8(cpu.regs.pc++);
    if (cpu.regs.z_flag()) {
        cpu.regs.pc += s8;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// 30 JR NC, s8
/* If the CY flag is 0, jump s8 steps from the current address stored in the program counter (PC). If not, the
 * instruction following the current JP instruction is executed (as usual). */
IMPL_INSTR(jr_nc_s8) {
    auto s8 = cpu.mmu.read_i8(cpu.regs.pc++);
    if (!cpu.regs.c_flag()) {
        cpu.regs.pc += s8;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// 38 JR C, s8
/* If the CY flag is 1, jump s8 steps from the current address stored in the program counter (PC). If not, the
 * instruction following the current JP instruction is executed (as usual). */
IMPL_INSTR(jr_c_s8) {
    auto s8 = cpu.mmu.read_i8(cpu.regs.pc++);
    if (cpu.regs.c_flag()) {
        cpu.regs.pc += s8;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// E9 JP HL
/* Load the contents of register pair HL into the program counter PC. The next instruction is fetched from the location
 * specified by the new value of PC. */
IMPL_INSTR(jp_hl) {
    cpu.regs.pc = cpu.regs.hl.u16;
}

// CD CALL a16
/* In memory, push the program counter PC value corresponding to the address following the CALL instruction to the 2
bytes following the byte specified by the current stack pointer SP. Then load the 16-bit immediate operand a16 into PC.
The subroutine is placed after the location specified by the new PC value. When the subroutine finishes, control is
returned to the source program using a return instruction and by popping the starting address of the next instruction
(which was just pushed) and moving it to the PC. With the push, the current value of SP is decremented by 1, and the
higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then
decremented by 1 again, and the lower-order byte of PC is loaded in the memory address specified by that value of SP.
The lower-order byte of a16 is placed in byte 2 of the object code, and the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_a16) {
    auto a16 = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    cpu.push(cpu.regs.pc);
    cpu.regs.pc = a16;
}

// C4 CALL NZ, a16
/* If the Z flag is 0, the program counter PC value corresponding to the memory location of the instruction following
the CALL instruction is pushed to the 2 bytes following the memory byte specified by the stack pointer SP. The 16-bit
immediate operand a16 is then loaded into PC. The lower-order byte of a16 is placed in byte 2 of the object code, and
the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_nz_a16) {
    auto a16 = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    if (!cpu.regs.z_flag()) {
        cpu.push(cpu.regs.pc);
        cpu.regs.pc = a16;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// CC CALL Z, a16
/* If the Z flag is 1, the program counter PC value corresponding to the memory location of the instruction following
the CALL instruction is pushed to the 2 bytes following the memory byte specified by the stack pointer SP. The 16-bit
immediate operand a16 is then loaded into PC. The lower-order byte of a16 is placed in byte 2 of the object code, and
the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_z_a16) {
    auto a16 = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    if (cpu.regs.z_flag()) {
        cpu.push(cpu.regs.pc);
        cpu.regs.pc = a16;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// D4 CALL NC, a16
/* If the CY flag is 0, the program counter PC value corresponding to the memory location of the instruction following
the CALL instruction is pushed to the 2 bytes following the memory byte specified by the stack pointer SP. The 16-bit
immediate operand a16 is then loaded into PC. The lower-order byte of a16 is placed in byte 2 of the object code, and
the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_nc_a16) {
    auto a16 = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    if (!cpu.regs.c_flag()) {
        cpu.push(cpu.regs.pc);
        cpu.regs.pc = a16;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// DC CALL C, a16
/* If the CY flag is 1, the program counter PC value corresponding to the memory location of the instruction following
the CALL instruction is pushed to the 2 bytes following the memory byte specified by the stack pointer SP. The 16-bit
immediate operand a16 is then loaded into PC. The lower-order byte of a16 is placed in byte 2 of the object code, and
the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_c_a16) {
    auto a16 = cpu.mmu.read_u16(cpu.regs.pc);
    cpu.regs.pc += 2;
    if (cpu.regs.c_flag()) {
        cpu.push(cpu.regs.pc);
        cpu.regs.pc = a16;
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// C9 RET
/* Pop from the memory stack the program counter PC value pushed when the subroutine was called, returning contorl to
the source program. The contents of the address specified by the stack pointer SP are loaded in the lower-order byte of
PC, and the contents of SP are incremented by 1. The contents of the address specified by the new SP value are then
loaded in the higher-order byte of PC, and the contents of SP are incremented by 1 again. (THe value of SP is 2 larger
than before instruction execution.) The next instruction is fetched from the address specified by the content of PC (as
usual). */
IMPL_INSTR(ret) {
    cpu.regs.pc = cpu.pop_u16();
}

// D9 RETI
/* Used when an interrupt-service routine finishes. The address for the return from the interrupt is loaded in the
program counter PC. The master interrupt enable flag is returned to its pre-interrupt status. The contents of the
address specified by the stack pointer SP are loaded in the lower-order byte of PC, and the contents of SP are
incremented by 1. The contents of the address specified by the new SP value are then loaded in the higher-order byte of
PC, and the contents of SP are incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.)
The next instruction is fetched from the address specified by the content of PC (as usual). */
IMPL_INSTR(reti) {
    cpu.regs.pc = cpu.pop_u16();
    cpu.interrupt_enabled = true;
}

// C0 RET NZ
/* If the Z flag is 0, control is returned to the source program by popping from the memory stack the program counter PC
value that was pushed to the stack when the subroutine was called. The contents of the address specified by the stack
pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the
address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are
incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched
from the address specified by the content of PC (as usual). */
IMPL_INSTR(ret_nz) {
    if (!cpu.regs.z_flag()) {
        cpu.regs.pc = cpu.pop_u16();
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// C8 RET Z
/* If the Z flag is 1, control is returned to the source program by popping from the memory stack the program counter PC
value that was pushed to the stack when the subroutine was called. The contents of the address specified by the stack
pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the
address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are
incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched
from the address specified by the content of PC (as usual). */
IMPL_INSTR(ret_z) {
    if (cpu.regs.z_flag()) {
        cpu.regs.pc = cpu.pop_u16();
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// D0 RET NC
/* If the CY flag is 0, control is returned to the source program by popping from the memory stack the program counter
PC value that was pushed to the stack when the subroutine was called. The contents of the address specified by the stack
pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the
address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are
incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched
from the address specified by the content of PC (as usual). */
IMPL_INSTR(ret_nc) {
    if (!cpu.regs.c_flag()) {
        cpu.regs.pc = cpu.pop_u16();
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// D8 RET C
/* If the CY flag is 1, control is returned to the source program by popping from the memory stack the program counter
PC value that was pushed to the stack when the subroutine was called. The contents of the address specified by the stack
pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the
address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are
incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched
from the address specified by the content of PC (as usual). */
IMPL_INSTR(ret_c) {
    if (cpu.regs.c_flag()) {
        cpu.regs.pc = cpu.pop_u16();
        cpu.extra_cycles = this->cycles_taken - this->cycles;
    }
}

// C7 RST 0
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 1th byte of page 0
memory addresses, 0x00. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in
the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order
byte of the PC is loaded in the memory address specified by that value of SP. The RST instruction can be used to jump to
1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the
PC, and 0x00 is loaded in the lower-order byte. */
IMPL_INSTR(rst_0) {
    cpu.push(cpu.regs.pc);
    cpu.regs.pc = 0x0000;
}

// CF RST 1
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 2th byte of page 0
memory addresses, 0x08. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in
the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order
byte of the PC is loaded in the memory address specified by that value of SP. The RST instruction can be used to jump to
1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the
PC, and 0x08 is loaded in the lower-order byte. */
IMPL_INSTR(rst_1) {
    cpu.push(cpu.regs.pc);
    cpu.regs.pc = 0x0008;
}

// D7 RST 2
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 3th byte of page 0
memory addresses, 0x10. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in
the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order
byte of the PC is loaded in the memory address specified by that value of SP. The RST instruction can be used to jump to
1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the
PC, and 0x10 is loaded in the lower-order byte. */
IMPL_INSTR(rst_2) {
    cpu.push(cpu.regs.pc);
    cpu.regs.pc = 0x0010;
}

// DF RST 3
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 4th byte of page 0
memory addresses, 0x18. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in
the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order
byte of the PC is loaded in the memory address specified by that value of SP. The RST instruction can be used to jump to
1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the
PC, and 0x18 is loaded in the lower-order byte. */
IMPL_INSTR(rst_3) {
    cpu.push(cpu.regs.pc);
    cpu.regs.pc = 0x0018;
}

// E7 RST 4
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 5th byte of page 0
memory addresses, 0x20. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in
the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order
byte of the PC is loaded in the memory address specified by that value of SP. The RST instruction can be used to jump to
1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the
PC, and 0x20 is loaded in the lower-order byte. */
IMPL_INSTR(rst_4) {
    cpu.push(cpu.regs.pc);
    cpu.regs.pc = 0x0020;
}

// EF RST 5
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 6th byte of page 0
memory addresses, 0x28. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in
the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order
byte of the PC is loaded in the memory address specified by that value of SP. The RST instruction can be used to jump to
1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the
PC, and 0x28 is loaded in the lower-order byte. */
IMPL_INSTR(rst_5) {
    cpu.push(cpu.regs.pc);
    cpu.regs.pc = 0x0028;
}

// F7 RST 6
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 7th byte of page 0
memory addresses, 0x30. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in
the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order
byte of the PC is loaded in the memory address specified by that value of SP. The RST instruction can be used to jump to
1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the
PC, and 0x30 is loaded in the lower-order byte. */
IMPL_INSTR(rst_6) {
    cpu.push(cpu.regs.pc);
    cpu.regs.pc = 0x0030;
}

// FF RST 7
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 8th byte of page 0
memory addresses, 0x38. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in
the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order
byte of the PC is loaded in the memory address specified by that value of SP. The RST instruction can be used to jump to
1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the
PC, and 0x38 is loaded in the lower-order byte. */
IMPL_INSTR(rst_7) {
    cpu.push(cpu.regs.pc);
    cpu.regs.pc = 0x0038;
}

// 27 DAA
/* Adjust the accumulator (register A) too a binary-coded decimal (BCD) number after BCD addition and subtraction
 * operations. */
IMPL_INSTR(daa) {
    cpu.regs.af.hi = cpu.alu.daa(cpu.regs.af.hi);
}

// 2F CPL
/* Take the one's complement (i.e., flip all bits) of the contents of register A. */
IMPL_INSTR(cpl) {
    cpu.regs.af.hi = cpu.alu.cpl(cpu.regs.af.hi);
}

// 00 NOP
/* Only advances the program counter by 1. Performs no other operations that would have an effect. */
IMPL_INSTR(nop) {}

// 3F CCF
/* Flip the carry flag CY. N=0, H=0, Z invariato. */
IMPL_INSTR(ccf) {
    cpu.regs.c_flag(!cpu.regs.c_flag());
    cpu.regs.n_flag(false);
    cpu.regs.h_flag(false);
}

// 37 SCF
/* Set the carry flag CY. N=0, H=0, Z invariato. */
IMPL_INSTR(scf) {
    cpu.regs.c_flag(true);
    cpu.regs.n_flag(false);
    cpu.regs.h_flag(false);
}

// F3 DI
/* Reset the interrupt master enable (IME) flag and prohibit maskable interrupts.
Even if a DI instruction is executed in an interrupt routine, the IME flag is set if a return is performed with a RETI
instruction. */
IMPL_INSTR(di) {
    cpu.interrupt_enabled = false;
    cpu.ime_pending = false;
}

// FB EI
/* Set the interrupt master enable (IME) flag and enable maskable interrupts. This instruction can be used in an
interrupt routine to enable higher-order interrupts. The IME flag is reset immediately after an interrupt occurs. The
IME flag reset remains in effect if coontrol is returned from the interrupt routine by a RET instruction. However, if an
EI instruction is executed in the interrupt routine, control is returned with IME = 1. */
IMPL_INSTR(ei) {
    cpu.ime_pending = true;
    cpu.ei_just_executed = true;
}

// 76 HALT
/* After a HALT instruction is executed, the system clock is stopped and HALT mode is entered. Although the system clock
is stopped in this status, the oscillator circuit and LCD controller continue to operate. In addition, the status of the
internal RAM register ports remains unchanged. HALT mode is cancelled by an interrupt or reset signal. The program
counter is halted at the step after the HALT instruction. If both the interrupt request flag and the corresponding
interrupt enable flag are set, HALT mode is exited, even if the interrupt master enable flag is not set. Once HALT mode
is cancelled, the program starts from the address indicated by the program counter. If the interrupt master enable flag
is set, the contents of the program coounter are pushed to the stack and control jumps to the starting address of the
interrupt. If the RESET terminal goes LOW in HALT moode, the mode becomes that of a normal reset. */
IMPL_INSTR(halt) {
    cpu.halted = true;
}

// 10 STOP
/* Execution of a STOP instruction stops both the system clock and oscillator circuit. STOP mode is entered and the LCD
controller also stops. However, the status of the internal RAM register ports remains unchanged. STOP mode can be
cancelled by a reset signal. If the RESET terminal goes LOW in STOP mode, it becomes that of a normal reset status. The
following conditions should be met before a STOP instruction is executed and stop mode is entered: All interrupt-enable
(IE) flags are reset. Input to P10-P13 is LOW for all. */
IMPL_INSTR(stop) {
    cpu.stopped = true;
}
