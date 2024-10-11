// Generated from ops_db.json
// https://gist.github.com/bberak/ca001281bb8431d2706afd31401e802b

#include <opcodes.hpp>
#include <exc.hpp>

#define IMPL_INSTR(x) \
	gbemu::instruction_types::##x gbemu::instructions::x##_{}; \
	void gbemu::instruction_types::##x##::execute(cpu& cpu)

std::unordered_map<std::uint16_t, gbemu::instruction&> gbemu::instruction_set;

// 7F LD A, A
/* Load the contents of register A into register A. */
IMPL_INSTR(ld_a_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 78 LD A, B
/* Load the contents of register B into register A. */
IMPL_INSTR(ld_a_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 79 LD A, C
/* Load the contents of register C into register A. */
IMPL_INSTR(ld_a_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 7A LD A, D
/* Load the contents of register D into register A. */
IMPL_INSTR(ld_a_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 7B LD A, E
/* Load the contents of register E into register A. */
IMPL_INSTR(ld_a_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 7C LD A, H
/* Load the contents of register H into register A. */
IMPL_INSTR(ld_a_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 7D LD A, L
/* Load the contents of register L into register A. */
IMPL_INSTR(ld_a_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 47 LD B, A
/* Load the contents of register A into register B. */
IMPL_INSTR(ld_b_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 40 LD B, B
/* Load the contents of register B into register B. */
IMPL_INSTR(ld_b_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 41 LD B, C
/* Load the contents of register C into register B. */
IMPL_INSTR(ld_b_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 42 LD B, D
/* Load the contents of register D into register B. */
IMPL_INSTR(ld_b_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 43 LD B, E
/* Load the contents of register E into register B. */
IMPL_INSTR(ld_b_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 44 LD B, H
/* Load the contents of register H into register B. */
IMPL_INSTR(ld_b_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 45 LD B, L
/* Load the contents of register L into register B. */
IMPL_INSTR(ld_b_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 4F LD C, A
/* Load the contents of register A into register C. */
IMPL_INSTR(ld_c_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 48 LD C, B
/* Load the contents of register B into register C. */
IMPL_INSTR(ld_c_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 49 LD C, C
/* Load the contents of register C into register C. */
IMPL_INSTR(ld_c_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 4A LD C, D
/* Load the contents of register D into register C. */
IMPL_INSTR(ld_c_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 4B LD C, E
/* Load the contents of register E into register C. */
IMPL_INSTR(ld_c_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 4C LD C, H
/* Load the contents of register H into register C. */
IMPL_INSTR(ld_c_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 4D LD C, L
/* Load the contents of register L into register C. */
IMPL_INSTR(ld_c_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 57 LD D, A
/* Load the contents of register A into register D. */
IMPL_INSTR(ld_d_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 50 LD D, B
/* Load the contents of register B into register D. */
IMPL_INSTR(ld_d_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 51 LD D, C
/* Load the contents of register C into register D. */
IMPL_INSTR(ld_d_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 52 LD D, D
/* Load the contents of register D into register D. */
IMPL_INSTR(ld_d_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 53 LD D, E
/* Load the contents of register E into register D. */
IMPL_INSTR(ld_d_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 54 LD D, H
/* Load the contents of register H into register D. */
IMPL_INSTR(ld_d_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 55 LD D, L
/* Load the contents of register L into register D. */
IMPL_INSTR(ld_d_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 5F LD E, A
/* Load the contents of register A into register E. */
IMPL_INSTR(ld_e_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 58 LD E, B
/* Load the contents of register B into register E. */
IMPL_INSTR(ld_e_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 59 LD E, C
/* Load the contents of register C into register E. */
IMPL_INSTR(ld_e_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 5A LD E, D
/* Load the contents of register D into register E. */
IMPL_INSTR(ld_e_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 5B LD E, E
/* Load the contents of register E into register E. */
IMPL_INSTR(ld_e_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 5C LD E, H
/* Load the contents of register H into register E. */
IMPL_INSTR(ld_e_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 5D LD E, L
/* Load the contents of register L into register E. */
IMPL_INSTR(ld_e_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 67 LD H, A
/* Load the contents of register A into register H. */
IMPL_INSTR(ld_h_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 60 LD H, B
/* Load the contents of register B into register H. */
IMPL_INSTR(ld_h_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 61 LD H, C
/* Load the contents of register C into register H. */
IMPL_INSTR(ld_h_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 62 LD H, D
/* Load the contents of register D into register H. */
IMPL_INSTR(ld_h_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 63 LD H, E
/* Load the contents of register E into register H. */
IMPL_INSTR(ld_h_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 64 LD H, H
/* Load the contents of register H into register H. */
IMPL_INSTR(ld_h_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 65 LD H, L
/* Load the contents of register L into register H. */
IMPL_INSTR(ld_h_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 6F LD L, A
/* Load the contents of register A into register L. */
IMPL_INSTR(ld_l_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 68 LD L, B
/* Load the contents of register B into register L. */
IMPL_INSTR(ld_l_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 69 LD L, C
/* Load the contents of register C into register L. */
IMPL_INSTR(ld_l_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 6A LD L, D
/* Load the contents of register D into register L. */
IMPL_INSTR(ld_l_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 6B LD L, E
/* Load the contents of register E into register L. */
IMPL_INSTR(ld_l_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 6C LD L, H
/* Load the contents of register H into register L. */
IMPL_INSTR(ld_l_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 6D LD L, L
/* Load the contents of register L into register L. */
IMPL_INSTR(ld_l_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 3E LD A, d8
/* Load the 8-bit immediate operand d8 into register A. */
IMPL_INSTR(ld_a_d8) {
	cpu.regs.af.hi = cpu.mmu.read_u8(cpu.regs.pc++);
}

// 06 LD B, d8
/* Load the 8-bit immediate operand d8 into register B. */
IMPL_INSTR(ld_b_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 0E LD C, d8
/* Load the 8-bit immediate operand d8 into register C. */
IMPL_INSTR(ld_c_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 16 LD D, d8
/* Load the 8-bit immediate operand d8 into register D. */
IMPL_INSTR(ld_d_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 1E LD E, d8
/* Load the 8-bit immediate operand d8 into register E. */
IMPL_INSTR(ld_e_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 26 LD H, d8
/* Load the 8-bit immediate operand d8 into register H. */
IMPL_INSTR(ld_h_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 2E LD L, d8
/* Load the 8-bit immediate operand d8 into register L. */
IMPL_INSTR(ld_l_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 7E LD A, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register A. */
IMPL_INSTR(ld_a__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 46 LD B, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register B. */
IMPL_INSTR(ld_b__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 4E LD C, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register C. */
IMPL_INSTR(ld_c__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 56 LD D, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register D. */
IMPL_INSTR(ld_d__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 5E LD E, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register E. */
IMPL_INSTR(ld_e__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 66 LD H, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register H. */
IMPL_INSTR(ld_h__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 6E LD L, (HL)
/* Load the 8-bit contents of memory specified by register pair HL into register L. */
IMPL_INSTR(ld_l__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 77 LD (HL), A
/* Store the contents of register A in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 70 LD (HL), B
/* Store the contents of register B in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 71 LD (HL), C
/* Store the contents of register C in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 72 LD (HL), D
/* Store the contents of register D in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 73 LD (HL), E
/* Store the contents of register E in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 74 LD (HL), H
/* Store the contents of register H in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 75 LD (HL), L
/* Store the contents of register L in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 36 LD (HL), d8
/* Store the contents of 8-bit immediate operand d8 in the memory location specified by register pair HL. */
IMPL_INSTR(ld__hl__d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 0A LD A, (BC)
/* Load the 8-bit contents of memory specified by register pair BC into register A. */
IMPL_INSTR(ld_a__bc_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 1A LD A, (DE)
/* Load the 8-bit contents of memory specified by register pair DE into register A. */
IMPL_INSTR(ld_a__de_) { throw gbemu::gbemu_exception{"not implemented"}; }

// F2 LD A, (C)
/* Load into register A the contents of the internal RAM, port register, or mode register at the address in the range 0xFF00-0xFFFF specified by register C.
0xFF00-0xFF7F: Port/Mode registers, control register, sound register
0xFF80-0xFFFE: Working & Stack RAM (127 bytes)
0xFFFF: Interrupt Enable Register */
IMPL_INSTR(ld_a__c_) { throw gbemu::gbemu_exception{"not implemented"}; }

// E2 LD (C), A
/* Store the contents of register A in the internal RAM, port register, or mode register at the address in the range 0xFF00-0xFFFF specified by register C.
0xFF00-0xFF7F: Port/Mode registers, control register, sound register
0xFF80-0xFFFE: Working & Stack RAM (127 bytes)
0xFFFF: Interrupt Enable Register */
IMPL_INSTR(ld__c__a) { throw gbemu::gbemu_exception{"not implemented"}; }

// F0 LD A, (a8)
/* Load into register A the contents of the internal RAM, port register, or mode register at the address in the range 0xFF00-0xFFFF specified by the 8-bit immediate operand a8.
Note: Should specify a 16-bit address in the mnemonic portion for a8, although the immediate operand only has the lower-order 8 bits.
0xFF00-0xFF7F: Port/Mode registers, control register, sound register
0xFF80-0xFFFE: Working & Stack RAM (127 bytes)
0xFFFF: Interrupt Enable Register */
IMPL_INSTR(ld_a__a8_) { throw gbemu::gbemu_exception{"not implemented"}; }

// E0 LD (a8), A
/* Store the contents of register A in the internal RAM, port register, or mode register at the address in the range 0xFF00-0xFFFF specified by the 8-bit immediate operand a8.
Note: Should specify a 16-bit address in the mnemonic portion for a8, although the immediate operand only has the lower-order 8 bits.
0xFF00-0xFF7F: Port/Mode registers, control register, sound register
0xFF80-0xFFFE: Working & Stack RAM (127 bytes)
0xFFFF: Interrupt Enable Register */
IMPL_INSTR(ld__a8__a) {
	std::uint16_t addr = cpu.mmu.read_u8(cpu.regs.pc++) | 0xFF00;
	cpu.mmu.write_u8(addr, cpu.regs.af.hi);
}

// FA LD A, (a16)
/* Load into register A the contents of the internal RAM or register specified by the 16-bit immediate operand a16. */
IMPL_INSTR(ld_a__a16_) { throw gbemu::gbemu_exception{"not implemented"}; }

// EA LD (a16), A
/* Store the contents of register A in the internal RAM or register specified by the 16-bit immediate operand a16. */
IMPL_INSTR(ld__a16__a) { 
	auto addr = cpu.mmu.read_u16(cpu.regs.pc);
	cpu.mmu.write_u8(addr, cpu.regs.af.hi);
	cpu.regs.pc += 2;
}

// 2A LD A, (HL+)
/* Load the contents of memory specified by register pair HL into register A, and simultaneously increment the contents of HL. */
IMPL_INSTR(ld_a__hlp_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 3A LD A, (HL-)
/* Load the contents of memory specified by register pair HL into register A, and simultaneously decrement the contents of HL. */
IMPL_INSTR(ld_a__hlm_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 02 LD (BC), A
/* Store the contents of register A in the memory location specified by register pair BC. */
IMPL_INSTR(ld__bc__a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 12 LD (DE), A
/* Store the contents of register A in the memory location specified by register pair DE. */
IMPL_INSTR(ld__de__a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 22 LD (HL+), A
/* Store the contents of register A into the memory location specified by register pair HL, and simultaneously increment the contents of HL. */
IMPL_INSTR(ld__hlp__a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 32 LD (HL-), A
/* Store the contents of register A into the memory location specified by register pair HL, and simultaneously decrement the contents of HL. */
IMPL_INSTR(ld__hlm__a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 01 LD BC, d16
/* Load the 2 bytes of immediate data into register pair BC.
 The first byte of immediate data is the lower byte (i.e., bits 0-7), and the second byte of immediate data is the higher byte (i.e., bits 8-15). */
IMPL_INSTR(ld_bc_d16) { throw gbemu::gbemu_exception{"not implemented"}; }

// 11 LD DE, d16
/* Load the 2 bytes of immediate data into register pair DE.
 The first byte of immediate data is the lower byte (i.e., bits 0-7), and the second byte of immediate data is the higher byte (i.e., bits 8-15). */
IMPL_INSTR(ld_de_d16) { throw gbemu::gbemu_exception{"not implemented"}; }

// 21 LD HL, d16
/* Load the 2 bytes of immediate data into register pair HL.
 The first byte of immediate data is the lower byte (i.e., bits 0-7), and the second byte of immediate data is the higher byte (i.e., bits 8-15). */
IMPL_INSTR(ld_hl_d16) { 
	auto nn = cpu.mmu.read_u16(cpu.regs.pc);
	cpu.regs.hl.u16 = nn;
	cpu.regs.pc += 2;
}

// 31 LD SP, d16
/* Load the 2 bytes of immediate data into register pair SP.
 The first byte of immediate data is the lower byte (i.e., bits 0-7), and the second byte of immediate data is the higher byte (i.e., bits 8-15). */
IMPL_INSTR(ld_sp_d16) { 
	auto nn = cpu.mmu.read_u16(cpu.regs.pc);
	cpu.regs.sp = nn;
	cpu.regs.pc += 2;
}

// F9 LD SP, HL
/* Load the contents of register pair HL into the stack pointer SP. */
IMPL_INSTR(ld_sp_hl) { throw gbemu::gbemu_exception{"not implemented"}; }

// C5 PUSH BC
/* Push the contents of register pair BC onto the memory stack by doing the following:
Subtract 1 from the stack pointer SP, and put the contents of the higher portion of register pair BC on the stack.
Subtract 2 from SP, and put the lower portion of register pair BC on the stack.
Decrement SP by 2. */
IMPL_INSTR(push_bc) { throw gbemu::gbemu_exception{"not implemented"}; }

// D5 PUSH DE
/* Push the contents of register pair DE onto the memory stack by doing the following:
Subtract 1 from the stack pointer SP, and put the contents of the higher portion of register pair DE on the stack.
Subtract 2 from SP, and put the lower portion of register pair DE on the stack.
Decrement SP by 2. */
IMPL_INSTR(push_de) { throw gbemu::gbemu_exception{"not implemented"}; }

// E5 PUSH HL
/* Push the contents of register pair HL onto the memory stack by doing the following:
Subtract 1 from the stack pointer SP, and put the contents of the higher portion of register pair HL on the stack.
Subtract 2 from SP, and put the lower portion of register pair HL on the stack.
Decrement SP by 2. */
IMPL_INSTR(push_hl) { throw gbemu::gbemu_exception{"not implemented"}; }

// F5 PUSH AF
/* Push the contents of register pair AF onto the memory stack by doing the following:
Subtract 1 from the stack pointer SP, and put the contents of the higher portion of register pair AF on the stack.
Subtract 2 from SP, and put the lower portion of register pair AF on the stack.
Decrement SP by 2. */
IMPL_INSTR(push_af) { throw gbemu::gbemu_exception{"not implemented"}; }

// C1 POP BC
/* Pop the contents from the memory stack into register pair into register pair BC by doing the following:
Load the contents of memory specified by stack pointer SP into the lower portion of BC.
Add 1 to SP and load the contents from the new memory location into the upper portion of BC.
By the end, SP should be 2 more than its initial value. */
IMPL_INSTR(pop_bc) { throw gbemu::gbemu_exception{"not implemented"}; }

// D1 POP DE
/* Pop the contents from the memory stack into register pair into register pair DE by doing the following:
Load the contents of memory specified by stack pointer SP into the lower portion of DE.
Add 1 to SP and load the contents from the new memory location into the upper portion of DE.
By the end, SP should be 2 more than its initial value. */
IMPL_INSTR(pop_de) { throw gbemu::gbemu_exception{"not implemented"}; }

// E1 POP HL
/* Pop the contents from the memory stack into register pair into register pair HL by doing the following:
Load the contents of memory specified by stack pointer SP into the lower portion of HL.
Add 1 to SP and load the contents from the new memory location into the upper portion of HL.
By the end, SP should be 2 more than its initial value. */
IMPL_INSTR(pop_hl) { throw gbemu::gbemu_exception{"not implemented"}; }

// F1 POP AF
/* Pop the contents from the memory stack into register pair into register pair AF by doing the following:
Load the contents of memory specified by stack pointer SP into the lower portion of AF.
Add 1 to SP and load the contents from the new memory location into the upper portion of AF.
By the end, SP should be 2 more than its initial value. */
IMPL_INSTR(pop_af) { throw gbemu::gbemu_exception{"not implemented"}; }

// F8 LD HL, SP+s8
/* Add the 8-bit signed operand s8 (values -128 to +127) to the stack pointer SP, and store the result in register pair HL. */
IMPL_INSTR(ld_hl_spps8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 08 LD (a16), SP
/* Store the lower byte of stack pointer SP at the address specified by the 16-bit immediate operand a16, and store the upper byte of SP at address a16 + 1. */
IMPL_INSTR(ld__a16__sp) { throw gbemu::gbemu_exception{"not implemented"}; }

// 87 ADD A, A
/* Add the contents of register A to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 80 ADD A, B
/* Add the contents of register B to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 81 ADD A, C
/* Add the contents of register C to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 82 ADD A, D
/* Add the contents of register D to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 83 ADD A, E
/* Add the contents of register E to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 84 ADD A, H
/* Add the contents of register H to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 85 ADD A, L
/* Add the contents of register L to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// C6 ADD A, d8
/* Add the contents of the 8-bit immediate operand d8 to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 86 ADD A, (HL)
/* Add the contents of memory specified by register pair HL to the contents of register A, and store the results in register A. */
IMPL_INSTR(add_a__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 8F ADC A, A
/* Add the contents of register A and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 88 ADC A, B
/* Add the contents of register B and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 89 ADC A, C
/* Add the contents of register C and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 8A ADC A, D
/* Add the contents of register D and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 8B ADC A, E
/* Add the contents of register E and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 8C ADC A, H
/* Add the contents of register H and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 8D ADC A, L
/* Add the contents of register L and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CE ADC A, d8
/* Add the contents of the 8-bit immediate operand d8 and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 8E ADC A, (HL)
/* Add the contents of memory specified by register pair HL and the CY flag to the contents of register A, and store the results in register A. */
IMPL_INSTR(adc_a__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 97 SUB A
/* Subtract the contents of register A from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 90 SUB B
/* Subtract the contents of register B from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 91 SUB C
/* Subtract the contents of register C from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 92 SUB D
/* Subtract the contents of register D from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 93 SUB E
/* Subtract the contents of register E from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 94 SUB H
/* Subtract the contents of register H from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 95 SUB L
/* Subtract the contents of register L from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// D6 SUB d8
/* Subtract the contents of the 8-bit immediate operand d8 from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 96 SUB (HL)
/* Subtract the contents of memory specified by register pair HL from the contents of register A, and store the results in register A. */
IMPL_INSTR(sub__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 9F SBC A, A
/* Subtract the contents of register A and the CY flag from the contents of register A, and store the results in register A. */
IMPL_INSTR(sbc_a_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 98 SBC A, B
/* Subtract the contents of register B and the CY flag from the contents of register A, and store the results in register A. */
IMPL_INSTR(sbc_a_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 99 SBC A, C
/* Subtract the contents of register C and the CY flag from the contents of register A, and store the results in register A. */
IMPL_INSTR(sbc_a_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 9A SBC A, D
/* Subtract the contents of register D and the CY flag from the contents of register A, and store the results in register A. */
IMPL_INSTR(sbc_a_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 9B SBC A, E
/* Subtract the contents of register E and the CY flag from the contents of register A, and store the results in register A. */
IMPL_INSTR(sbc_a_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 9C SBC A, H
/* Subtract the contents of register H and the CY flag from the contents of register A, and store the results in register A. */
IMPL_INSTR(sbc_a_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 9D SBC A, L
/* Subtract the contents of register L and the CY flag from the contents of register A, and store the results in register A. */
IMPL_INSTR(sbc_a_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// DE SBC A, d8
/* Subtract the contents of the 8-bit immediate operand d8 and the carry flag CY from the contents of register A, and store the results in register A. */
IMPL_INSTR(sbc_a_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 9E SBC A, (HL)
/* Subtract the contents of memory specified by register pair HL and the carry flag CY from the contents of register A, and store the results in register A. */
IMPL_INSTR(sbc_a__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// A7 AND A
/* Take the logical AND for each bit of the contents of register A and the contents of register A, and store the results in register A. */
IMPL_INSTR(and_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// A0 AND B
/* Take the logical AND for each bit of the contents of register B and the contents of register A, and store the results in register A. */
IMPL_INSTR(and_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// A1 AND C
/* Take the logical AND for each bit of the contents of register C and the contents of register A, and store the results in register A. */
IMPL_INSTR(and_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// A2 AND D
/* Take the logical AND for each bit of the contents of register D and the contents of register A, and store the results in register A. */
IMPL_INSTR(and_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// A3 AND E
/* Take the logical AND for each bit of the contents of register E and the contents of register A, and store the results in register A. */
IMPL_INSTR(and_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// A4 AND H
/* Take the logical AND for each bit of the contents of register H and the contents of register A, and store the results in register A. */
IMPL_INSTR(and_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// A5 AND L
/* Take the logical AND for each bit of the contents of register L and the contents of register A, and store the results in register A. */
IMPL_INSTR(and_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// E6 AND d8
/* Take the logical AND for each bit of the contents of 8-bit immediate operand d8 and the contents of register A, and store the results in register A. */
IMPL_INSTR(and_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// A6 AND (HL)
/* Take the logical AND for each bit of the contents of memory specified by register pair HL and the contents of register A, and store the results in register A. */
IMPL_INSTR(and__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// B7 OR A
/* Take the logical OR for each bit of the contents of register A and the contents of register A, and store the results in register A. */
IMPL_INSTR(or_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// B0 OR B
/* Take the logical OR for each bit of the contents of register B and the contents of register A, and store the results in register A. */
IMPL_INSTR(or_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// B1 OR C
/* Take the logical OR for each bit of the contents of register C and the contents of register A, and store the results in register A. */
IMPL_INSTR(or_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// B2 OR D
/* Take the logical OR for each bit of the contents of register D and the contents of register A, and store the results in register A. */
IMPL_INSTR(or_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// B3 OR E
/* Take the logical OR for each bit of the contents of register E and the contents of register A, and store the results in register A. */
IMPL_INSTR(or_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// B4 OR H
/* Take the logical OR for each bit of the contents of register H and the contents of register A, and store the results in register A. */
IMPL_INSTR(or_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// B5 OR L
/* Take the logical OR for each bit of the contents of register L and the contents of register A, and store the results in register A. */
IMPL_INSTR(or_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// F6 OR d8
/* Take the logical OR for each bit of the contents of the 8-bit immediate operand d8 and the contents of register A, and store the results in register A. */
IMPL_INSTR(or_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// B6 OR (HL)
/* Take the logical OR for each bit of the contents of memory specified by register pair HL and the contents of register A, and store the results in register A. */
IMPL_INSTR(or__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// AF XOR A
/* Take the logical exclusive-OR for each bit of the contents of register A and the contents of register A, and store the results in register A. */
IMPL_INSTR(xor_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// A8 XOR B
/* Take the logical exclusive-OR for each bit of the contents of register B and the contents of register A, and store the results in register A. */
IMPL_INSTR(xor_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// A9 XOR C
/* Take the logical exclusive-OR for each bit of the contents of register C and the contents of register A, and store the results in register A. */
IMPL_INSTR(xor_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// AA XOR D
/* Take the logical exclusive-OR for each bit of the contents of register D and the contents of register A, and store the results in register A. */
IMPL_INSTR(xor_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// AB XOR E
/* Take the logical exclusive-OR for each bit of the contents of register E and the contents of register A, and store the results in register A. */
IMPL_INSTR(xor_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// AC XOR H
/* Take the logical exclusive-OR for each bit of the contents of register H and the contents of register A, and store the results in register A. */
IMPL_INSTR(xor_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// AD XOR L
/* Take the logical exclusive-OR for each bit of the contents of register L and the contents of register A, and store the results in register A. */
IMPL_INSTR(xor_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// EE XOR d8
/* Take the logical exclusive-OR for each bit of the contents of the 8-bit immediate operand d8 and the contents of register A, and store the results in register A. */
IMPL_INSTR(xor_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// AE XOR (HL)
/* Take the logical exclusive-OR for each bit of the contents of memory specified by register pair HL and the contents of register A, and store the results in register A. */
IMPL_INSTR(xor__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// BF CP A
/* Compare the contents of register A and the contents of register A by calculating A - A, and set the Z flag if they are equal.
The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// B8 CP B
/* Compare the contents of register B and the contents of register A by calculating A - B, and set the Z flag if they are equal.
The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// B9 CP C
/* Compare the contents of register C and the contents of register A by calculating A - C, and set the Z flag if they are equal.
The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// BA CP D
/* Compare the contents of register D and the contents of register A by calculating A - D, and set the Z flag if they are equal.
The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// BB CP E
/* Compare the contents of register E and the contents of register A by calculating A - E, and set the Z flag if they are equal.
The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// BC CP H
/* Compare the contents of register H and the contents of register A by calculating A - H, and set the Z flag if they are equal.
The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// BD CP L
/* Compare the contents of register L and the contents of register A by calculating A - L, and set the Z flag if they are equal.
The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// FE CP d8
/* Compare the contents of register A and the contents of the 8-bit immediate operand d8 by calculating A - d8, and set the Z flag if they are equal.
The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp_d8) { throw gbemu::gbemu_exception{"not implemented"}; }

// BE CP (HL)
/* Compare the contents of memory specified by register pair HL and the contents of register A by calculating A - (HL), and set the Z flag if they are equal.
The execution of this instruction does not affect the contents of register A. */
IMPL_INSTR(cp__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 3C INC A
/* Increment the contents of register A by 1. */
IMPL_INSTR(inc_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 04 INC B
/* Increment the contents of register B by 1. */
IMPL_INSTR(inc_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 0C INC C
/* Increment the contents of register C by 1. */
IMPL_INSTR(inc_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 14 INC D
/* Increment the contents of register D by 1. */
IMPL_INSTR(inc_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 1C INC E
/* Increment the contents of register E by 1. */
IMPL_INSTR(inc_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 24 INC H
/* Increment the contents of register H by 1. */
IMPL_INSTR(inc_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 2C INC L
/* Increment the contents of register L by 1. */
IMPL_INSTR(inc_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 34 INC (HL)
/* Increment the contents of memory specified by register pair HL by 1. */
IMPL_INSTR(inc__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 3D DEC A
/* Decrement the contents of register A by 1. */
IMPL_INSTR(dec_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// 05 DEC B
/* Decrement the contents of register B by 1. */
IMPL_INSTR(dec_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// 0D DEC C
/* Decrement the contents of register C by 1. */
IMPL_INSTR(dec_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// 15 DEC D
/* Decrement the contents of register D by 1. */
IMPL_INSTR(dec_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// 1D DEC E
/* Decrement the contents of register E by 1. */
IMPL_INSTR(dec_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// 25 DEC H
/* Decrement the contents of register H by 1. */
IMPL_INSTR(dec_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// 2D DEC L
/* Decrement the contents of register L by 1. */
IMPL_INSTR(dec_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// 35 DEC (HL)
/* Decrement the contents of memory specified by register pair HL by 1. */
IMPL_INSTR(dec__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// 09 ADD HL, BC
/* Add the contents of register pair BC to the contents of register pair HL, and store the results in register pair HL. */
IMPL_INSTR(add_hl_bc) { throw gbemu::gbemu_exception{"not implemented"}; }

// 19 ADD HL, DE
/* Add the contents of register pair DE to the contents of register pair HL, and store the results in register pair HL. */
IMPL_INSTR(add_hl_de) { throw gbemu::gbemu_exception{"not implemented"}; }

// 29 ADD HL, HL
/* Add the contents of register pair HL to the contents of register pair HL, and store the results in register pair HL. */
IMPL_INSTR(add_hl_hl) { throw gbemu::gbemu_exception{"not implemented"}; }

// 39 ADD HL, SP
/* Add the contents of register pair SP to the contents of register pair HL, and store the results in register pair HL. */
IMPL_INSTR(add_hl_sp) { throw gbemu::gbemu_exception{"not implemented"}; }

// E8 ADD SP, s8
/* Add the contents of the 8-bit signed (2's complement) immediate operand s8 and the stack pointer SP and store the results in SP. */
IMPL_INSTR(add_sp_s8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 03 INC BC
/* Increment the contents of register pair BC by 1. */
IMPL_INSTR(inc_bc) { throw gbemu::gbemu_exception{"not implemented"}; }

// 13 INC DE
/* Increment the contents of register pair DE by 1. */
IMPL_INSTR(inc_de) { throw gbemu::gbemu_exception{"not implemented"}; }

// 23 INC HL
/* Increment the contents of register pair HL by 1. */
IMPL_INSTR(inc_hl) { throw gbemu::gbemu_exception{"not implemented"}; }

// 33 INC SP
/* Increment the contents of register pair SP by 1. */
IMPL_INSTR(inc_sp) { throw gbemu::gbemu_exception{"not implemented"}; }

// 0B DEC BC
/* Decrement the contents of register pair BC by 1. */
IMPL_INSTR(dec_bc) { throw gbemu::gbemu_exception{"not implemented"}; }

// 1B DEC DE
/* Decrement the contents of register pair DE by 1. */
IMPL_INSTR(dec_de) { throw gbemu::gbemu_exception{"not implemented"}; }

// 2B DEC HL
/* Decrement the contents of register pair HL by 1. */
IMPL_INSTR(dec_hl) { throw gbemu::gbemu_exception{"not implemented"}; }

// 3B DEC SP
/* Decrement the contents of register pair SP by 1. */
IMPL_INSTR(dec_sp) { throw gbemu::gbemu_exception{"not implemented"}; }

// 07 RLCA
/* Rotate the contents of register A to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register A. */
IMPL_INSTR(rlca) { throw gbemu::gbemu_exception{"not implemented"}; }

// 17 RLA
/* Rotate the contents of register A to the left, through the carry (CY) flag. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry flag are copied to bit 0. */
IMPL_INSTR(rla) { throw gbemu::gbemu_exception{"not implemented"}; }

// 0F RRCA
/* Rotate the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register A. */
IMPL_INSTR(rrca) { throw gbemu::gbemu_exception{"not implemented"}; }

// 1F RRA
/* Rotate the contents of register A to the right, through the carry (CY) flag. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry flag are copied to bit 7. */
IMPL_INSTR(rra) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB07 RLC A
/* Rotate the contents of register A to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register A. */
IMPL_INSTR(rlc_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB00 RLC B
/* Rotate the contents of register B to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register B. */
IMPL_INSTR(rlc_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB01 RLC C
/* Rotate the contents of register C to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register C. */
IMPL_INSTR(rlc_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB02 RLC D
/* Rotate the contents of register D to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register D. */
IMPL_INSTR(rlc_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB03 RLC E
/* Rotate the contents of register E to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register E. */
IMPL_INSTR(rlc_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB04 RLC H
/* Rotate the contents of register H to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register H. */
IMPL_INSTR(rlc_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB05 RLC L
/* Rotate the contents of register L to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are placed in both the CY flag and bit 0 of register L. */
IMPL_INSTR(rlc_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB06 RLC (HL)
/* Rotate the contents of memory specified by register pair HL to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the memory location. The contents of bit 7 are placed in both the CY flag and bit 0 of (HL). */
IMPL_INSTR(rlc__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB17 RL A
/* Rotate the contents of register A to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register A. */
IMPL_INSTR(rl_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB10 RL B
/* Rotate the contents of register B to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register B. */
IMPL_INSTR(rl_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB11 RL C
/* Rotate the contents of register C to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register C. */
IMPL_INSTR(rl_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB12 RL D
/* Rotate the contents of register D to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register D. */
IMPL_INSTR(rl_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB13 RL E
/* Rotate the contents of register E to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register E. */
IMPL_INSTR(rl_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB14 RL H
/* Rotate the contents of register H to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register H. */
IMPL_INSTR(rl_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB15 RL L
/* Rotate the contents of register L to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 0 of register L. */
IMPL_INSTR(rl_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB16 RL (HL)
/* Rotate the contents of memory specified by register pair HL to the left, through the carry flag. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the memory location. The previous contents of the CY flag are copied into bit 0 of (HL). */
IMPL_INSTR(rl__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB0F RRC A
/* Rotate the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register A. */
IMPL_INSTR(rrc_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB08 RRC B
/* Rotate the contents of register B to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register B. */
IMPL_INSTR(rrc_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB09 RRC C
/* Rotate the contents of register C to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register C. */
IMPL_INSTR(rrc_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB0A RRC D
/* Rotate the contents of register D to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register D. */
IMPL_INSTR(rrc_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB0B RRC E
/* Rotate the contents of register E to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register E. */
IMPL_INSTR(rrc_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB0C RRC H
/* Rotate the contents of register H to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register H. */
IMPL_INSTR(rrc_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB0D RRC L
/* Rotate the contents of register L to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are placed in both the CY flag and bit 7 of register L. */
IMPL_INSTR(rrc_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB0E RRC (HL)
/* Rotate the contents of memory specified by register pair HL to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the memory location. The contents of bit 0 are placed in both the CY flag and bit 7 of (HL). */
IMPL_INSTR(rrc__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB1F RR A
/* Rotate the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register A. */
IMPL_INSTR(rr_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB18 RR B
/* Rotate the contents of register B to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register B. */
IMPL_INSTR(rr_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB19 RR C
/* Rotate the contents of register C to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register C. */
IMPL_INSTR(rr_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB1A RR D
/* Rotate the contents of register D to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register D. */
IMPL_INSTR(rr_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB1B RR E
/* Rotate the contents of register E to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register E. */
IMPL_INSTR(rr_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB1C RR H
/* Rotate the contents of register H to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register H. */
IMPL_INSTR(rr_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB1D RR L
/* Rotate the contents of register L to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The previous contents of the carry (CY) flag are copied to bit 7 of register L. */
IMPL_INSTR(rr_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB1E RR (HL)
/* Rotate the contents of memory specified by register pair HL to the right, through the carry flag. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the memory location. The previous contents of the CY flag are copied into bit 7 of (HL). */
IMPL_INSTR(rr__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB27 SLA A
/* Shift the contents of register A to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register A is reset to 0. */
IMPL_INSTR(sla_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB20 SLA B
/* Shift the contents of register B to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register B is reset to 0. */
IMPL_INSTR(sla_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB21 SLA C
/* Shift the contents of register C to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register C is reset to 0. */
IMPL_INSTR(sla_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB22 SLA D
/* Shift the contents of register D to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register D is reset to 0. */
IMPL_INSTR(sla_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB23 SLA E
/* Shift the contents of register E to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register E is reset to 0. */
IMPL_INSTR(sla_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB24 SLA H
/* Shift the contents of register H to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register H is reset to 0. */
IMPL_INSTR(sla_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB25 SLA L
/* Shift the contents of register L to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the register. The contents of bit 7 are copied to the CY flag, and bit 0 of register L is reset to 0. */
IMPL_INSTR(sla_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB26 SLA (HL)
/* Shift the contents of memory specified by register pair HL to the left. That is, the contents of bit 0 are copied to bit 1, and the previous contents of bit 1 (before the copy operation) are copied to bit 2. The same operation is repeated in sequence for the rest of the memory location. The contents of bit 7 are copied to the CY flag, and bit 0 of (HL) is reset to 0. */
IMPL_INSTR(sla__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB2F SRA A
/* Shift the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register A is unchanged. */
IMPL_INSTR(sra_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB28 SRA B
/* Shift the contents of register B to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register B is unchanged. */
IMPL_INSTR(sra_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB29 SRA C
/* Shift the contents of register C to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register C is unchanged. */
IMPL_INSTR(sra_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB2A SRA D
/* Shift the contents of register D to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register D is unchanged. */
IMPL_INSTR(sra_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB2B SRA E
/* Shift the contents of register E to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register E is unchanged. */
IMPL_INSTR(sra_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB2C SRA H
/* Shift the contents of register H to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register H is unchanged. */
IMPL_INSTR(sra_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB2D SRA L
/* Shift the contents of register L to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register L is unchanged. */
IMPL_INSTR(sra_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB2E SRA (HL)
/* Shift the contents of memory specified by register pair HL to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the memory location. The contents of bit 0 are copied to the CY flag, and bit 7 of (HL) is unchanged. */
IMPL_INSTR(sra__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB3F SRL A
/* Shift the contents of register A to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register A is reset to 0. */
IMPL_INSTR(srl_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB38 SRL B
/* Shift the contents of register B to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register B is reset to 0. */
IMPL_INSTR(srl_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB39 SRL C
/* Shift the contents of register C to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register C is reset to 0. */
IMPL_INSTR(srl_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB3A SRL D
/* Shift the contents of register D to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register D is reset to 0. */
IMPL_INSTR(srl_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB3B SRL E
/* Shift the contents of register E to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register E is reset to 0. */
IMPL_INSTR(srl_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB3C SRL H
/* Shift the contents of register H to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register H is reset to 0. */
IMPL_INSTR(srl_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB3D SRL L
/* Shift the contents of register L to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the register. The contents of bit 0 are copied to the CY flag, and bit 7 of register L is reset to 0. */
IMPL_INSTR(srl_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB3E SRL (HL)
/* Shift the contents of memory specified by register pair HL to the right. That is, the contents of bit 7 are copied to bit 6, and the previous contents of bit 6 (before the copy operation) are copied to bit 5. The same operation is repeated in sequence for the rest of the memory location. The contents of bit 0 are copied to the CY flag, and bit 7 of (HL) is reset to 0. */
IMPL_INSTR(srl__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB37 SWAP A
/* Shift the contents of the lower-order four bits (0-3) of register A to the higher-order four bits (4-7) of the register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB30 SWAP B
/* Shift the contents of the lower-order four bits (0-3) of register B to the higher-order four bits (4-7) of the register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB31 SWAP C
/* Shift the contents of the lower-order four bits (0-3) of register C to the higher-order four bits (4-7) of the register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB32 SWAP D
/* Shift the contents of the lower-order four bits (0-3) of register D to the higher-order four bits (4-7) of the register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB33 SWAP E
/* Shift the contents of the lower-order four bits (0-3) of register E to the higher-order four bits (4-7) of the register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB34 SWAP H
/* Shift the contents of the lower-order four bits (0-3) of register H to the higher-order four bits (4-7) of the register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB35 SWAP L
/* Shift the contents of the lower-order four bits (0-3) of register L to the higher-order four bits (4-7) of the register, and shift the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB36 SWAP (HL)
/* Shift the contents of the lower-order four bits (0-3) of the contents of memory specified by register pair HL to the higher-order four bits (4-7) of that memory location, and shift the contents of the higher-order four bits to the lower-order four bits. */
IMPL_INSTR(swap__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB47 BIT 0, A
/* Copy the complement of the contents of bit 0 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB40 BIT 0, B
/* Copy the complement of the contents of bit 0 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB41 BIT 0, C
/* Copy the complement of the contents of bit 0 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB42 BIT 0, D
/* Copy the complement of the contents of bit 0 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB43 BIT 0, E
/* Copy the complement of the contents of bit 0 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB44 BIT 0, H
/* Copy the complement of the contents of bit 0 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB45 BIT 0, L
/* Copy the complement of the contents of bit 0 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB4F BIT 1, A
/* Copy the complement of the contents of bit 1 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB48 BIT 1, B
/* Copy the complement of the contents of bit 1 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB49 BIT 1, C
/* Copy the complement of the contents of bit 1 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB4A BIT 1, D
/* Copy the complement of the contents of bit 1 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB4B BIT 1, E
/* Copy the complement of the contents of bit 1 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB4C BIT 1, H
/* Copy the complement of the contents of bit 1 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB4D BIT 1, L
/* Copy the complement of the contents of bit 1 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB57 BIT 2, A
/* Copy the complement of the contents of bit 2 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB50 BIT 2, B
/* Copy the complement of the contents of bit 2 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB51 BIT 2, C
/* Copy the complement of the contents of bit 2 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB52 BIT 2, D
/* Copy the complement of the contents of bit 2 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB53 BIT 2, E
/* Copy the complement of the contents of bit 2 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB54 BIT 2, H
/* Copy the complement of the contents of bit 2 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB55 BIT 2, L
/* Copy the complement of the contents of bit 2 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB5F BIT 3, A
/* Copy the complement of the contents of bit 3 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB58 BIT 3, B
/* Copy the complement of the contents of bit 3 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB59 BIT 3, C
/* Copy the complement of the contents of bit 3 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB5A BIT 3, D
/* Copy the complement of the contents of bit 3 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB5B BIT 3, E
/* Copy the complement of the contents of bit 3 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB5C BIT 3, H
/* Copy the complement of the contents of bit 3 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB5D BIT 3, L
/* Copy the complement of the contents of bit 3 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB67 BIT 4, A
/* Copy the complement of the contents of bit 4 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB60 BIT 4, B
/* Copy the complement of the contents of bit 4 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB61 BIT 4, C
/* Copy the complement of the contents of bit 4 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB62 BIT 4, D
/* Copy the complement of the contents of bit 4 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB63 BIT 4, E
/* Copy the complement of the contents of bit 4 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB64 BIT 4, H
/* Copy the complement of the contents of bit 4 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB65 BIT 4, L
/* Copy the complement of the contents of bit 4 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB6F BIT 5, A
/* Copy the complement of the contents of bit 5 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB68 BIT 5, B
/* Copy the complement of the contents of bit 5 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB69 BIT 5, C
/* Copy the complement of the contents of bit 5 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB6A BIT 5, D
/* Copy the complement of the contents of bit 5 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB6B BIT 5, E
/* Copy the complement of the contents of bit 5 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB6C BIT 5, H
/* Copy the complement of the contents of bit 5 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB6D BIT 5, L
/* Copy the complement of the contents of bit 5 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB77 BIT 6, A
/* Copy the complement of the contents of bit 6 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB70 BIT 6, B
/* Copy the complement of the contents of bit 6 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB71 BIT 6, C
/* Copy the complement of the contents of bit 6 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB72 BIT 6, D
/* Copy the complement of the contents of bit 6 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB73 BIT 6, E
/* Copy the complement of the contents of bit 6 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB74 BIT 6, H
/* Copy the complement of the contents of bit 6 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB75 BIT 6, L
/* Copy the complement of the contents of bit 6 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB7F BIT 7, A
/* Copy the complement of the contents of bit 7 in register A to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB78 BIT 7, B
/* Copy the complement of the contents of bit 7 in register B to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB79 BIT 7, C
/* Copy the complement of the contents of bit 7 in register C to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB7A BIT 7, D
/* Copy the complement of the contents of bit 7 in register D to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB7B BIT 7, E
/* Copy the complement of the contents of bit 7 in register E to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB7C BIT 7, H
/* Copy the complement of the contents of bit 7 in register H to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB7D BIT 7, L
/* Copy the complement of the contents of bit 7 in register L to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB46 BIT 0, (HL)
/* Copy the complement of the contents of bit 0 in the memory location specified by register pair HL to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_0__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB4E BIT 1, (HL)
/* Copy the complement of the contents of bit 1 in the memory location specified by register pair HL to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_1__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB56 BIT 2, (HL)
/* Copy the complement of the contents of bit 2 in the memory location specified by register pair HL to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_2__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB5E BIT 3, (HL)
/* Copy the complement of the contents of bit 3 in the memory location specified by register pair HL to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_3__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB66 BIT 4, (HL)
/* Copy the complement of the contents of bit 4 in the memory location specified by register pair HL to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_4__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB6E BIT 5, (HL)
/* Copy the complement of the contents of bit 5 in the memory location specified by register pair HL to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_5__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB76 BIT 6, (HL)
/* Copy the complement of the contents of bit 6 in the memory location specified by register pair HL to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_6__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB7E BIT 7, (HL)
/* Copy the complement of the contents of bit 7 in the memory location specified by register pair HL to the Z flag of the program status word (PSW). */
IMPL_INSTR(bit_7__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC7 SET 0, A
/* Set bit 0 in register A to 1. */
IMPL_INSTR(set_0_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC0 SET 0, B
/* Set bit 0 in register B to 1. */
IMPL_INSTR(set_0_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC1 SET 0, C
/* Set bit 0 in register C to 1. */
IMPL_INSTR(set_0_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC2 SET 0, D
/* Set bit 0 in register D to 1. */
IMPL_INSTR(set_0_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC3 SET 0, E
/* Set bit 0 in register E to 1. */
IMPL_INSTR(set_0_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC4 SET 0, H
/* Set bit 0 in register H to 1. */
IMPL_INSTR(set_0_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC5 SET 0, L
/* Set bit 0 in register L to 1. */
IMPL_INSTR(set_0_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBCF SET 1, A
/* Set bit 1 in register A to 1. */
IMPL_INSTR(set_1_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC8 SET 1, B
/* Set bit 1 in register B to 1. */
IMPL_INSTR(set_1_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC9 SET 1, C
/* Set bit 1 in register C to 1. */
IMPL_INSTR(set_1_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBCA SET 1, D
/* Set bit 1 in register D to 1. */
IMPL_INSTR(set_1_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBCB SET 1, E
/* Set bit 1 in register E to 1. */
IMPL_INSTR(set_1_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBCC SET 1, H
/* Set bit 1 in register H to 1. */
IMPL_INSTR(set_1_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBCD SET 1, L
/* Set bit 1 in register L to 1. */
IMPL_INSTR(set_1_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD7 SET 2, A
/* Set bit 2 in register A to 1. */
IMPL_INSTR(set_2_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD0 SET 2, B
/* Set bit 2 in register B to 1. */
IMPL_INSTR(set_2_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD1 SET 2, C
/* Set bit 2 in register C to 1. */
IMPL_INSTR(set_2_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD2 SET 2, D
/* Set bit 2 in register D to 1. */
IMPL_INSTR(set_2_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD3 SET 2, E
/* Set bit 2 in register E to 1. */
IMPL_INSTR(set_2_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD4 SET 2, H
/* Set bit 2 in register H to 1. */
IMPL_INSTR(set_2_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD5 SET 2, L
/* Set bit 2 in register L to 1. */
IMPL_INSTR(set_2_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBDF SET 3, A
/* Set bit 3 in register A to 1. */
IMPL_INSTR(set_3_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD8 SET 3, B
/* Set bit 3 in register B to 1. */
IMPL_INSTR(set_3_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD9 SET 3, C
/* Set bit 3 in register C to 1. */
IMPL_INSTR(set_3_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBDA SET 3, D
/* Set bit 3 in register D to 1. */
IMPL_INSTR(set_3_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBDB SET 3, E
/* Set bit 3 in register E to 1. */
IMPL_INSTR(set_3_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBDC SET 3, H
/* Set bit 3 in register H to 1. */
IMPL_INSTR(set_3_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBDD SET 3, L
/* Set bit 3 in register L to 1. */
IMPL_INSTR(set_3_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE7 SET 4, A
/* Set bit 4 in register A to 1. */
IMPL_INSTR(set_4_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE0 SET 4, B
/* Set bit 4 in register B to 1. */
IMPL_INSTR(set_4_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE1 SET 4, C
/* Set bit 4 in register C to 1. */
IMPL_INSTR(set_4_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE2 SET 4, D
/* Set bit 4 in register D to 1. */
IMPL_INSTR(set_4_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE3 SET 4, E
/* Set bit 4 in register E to 1. */
IMPL_INSTR(set_4_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE4 SET 4, H
/* Set bit 4 in register H to 1. */
IMPL_INSTR(set_4_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE5 SET 4, L
/* Set bit 4 in register L to 1. */
IMPL_INSTR(set_4_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBEF SET 5, A
/* Set bit 5 in register A to 1. */
IMPL_INSTR(set_5_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE8 SET 5, B
/* Set bit 5 in register B to 1. */
IMPL_INSTR(set_5_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE9 SET 5, C
/* Set bit 5 in register C to 1. */
IMPL_INSTR(set_5_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBEA SET 5, D
/* Set bit 5 in register D to 1. */
IMPL_INSTR(set_5_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBEB SET 5, E
/* Set bit 5 in register E to 1. */
IMPL_INSTR(set_5_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBEC SET 5, H
/* Set bit 5 in register H to 1. */
IMPL_INSTR(set_5_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBED SET 5, L
/* Set bit 5 in register L to 1. */
IMPL_INSTR(set_5_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF7 SET 6, A
/* Set bit 6 in register A to 1. */
IMPL_INSTR(set_6_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF0 SET 6, B
/* Set bit 6 in register B to 1. */
IMPL_INSTR(set_6_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF1 SET 6, C
/* Set bit 6 in register C to 1. */
IMPL_INSTR(set_6_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF2 SET 6, D
/* Set bit 6 in register D to 1. */
IMPL_INSTR(set_6_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF3 SET 6, E
/* Set bit 6 in register E to 1. */
IMPL_INSTR(set_6_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF4 SET 6, H
/* Set bit 6 in register H to 1. */
IMPL_INSTR(set_6_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF5 SET 6, L
/* Set bit 6 in register L to 1. */
IMPL_INSTR(set_6_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBFF SET 7, A
/* Set bit 7 in register A to 1. */
IMPL_INSTR(set_7_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF8 SET 7, B
/* Set bit 7 in register B to 1. */
IMPL_INSTR(set_7_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF9 SET 7, C
/* Set bit 7 in register C to 1. */
IMPL_INSTR(set_7_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBFA SET 7, D
/* Set bit 7 in register D to 1. */
IMPL_INSTR(set_7_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBFB SET 7, E
/* Set bit 7 in register E to 1. */
IMPL_INSTR(set_7_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBFC SET 7, H
/* Set bit 7 in register H to 1. */
IMPL_INSTR(set_7_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBFD SET 7, L
/* Set bit 7 in register L to 1. */
IMPL_INSTR(set_7_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBC6 SET 0, (HL)
/* Set bit 0 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_0__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBCE SET 1, (HL)
/* Set bit 1 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_1__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBD6 SET 2, (HL)
/* Set bit 2 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_2__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBDE SET 3, (HL)
/* Set bit 3 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_3__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBE6 SET 4, (HL)
/* Set bit 4 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_4__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBEE SET 5, (HL)
/* Set bit 5 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_5__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBF6 SET 6, (HL)
/* Set bit 6 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_6__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBFE SET 7, (HL)
/* Set bit 7 in the memory location specified by register pair HL to 1. */
IMPL_INSTR(set_7__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB87 RES 0, A
/* Reset bit 0 in register A to 0. */
IMPL_INSTR(res_0_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB80 RES 0, B
/* Reset bit 0 in register B to 0. */
IMPL_INSTR(res_0_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB81 RES 0, C
/* Reset bit 0 in register C to 0. */
IMPL_INSTR(res_0_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB82 RES 0, D
/* Reset bit 0 in register D to 0. */
IMPL_INSTR(res_0_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB83 RES 0, E
/* Reset bit 0 in register E to 0. */
IMPL_INSTR(res_0_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB84 RES 0, H
/* Reset bit 0 in register H to 0. */
IMPL_INSTR(res_0_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB85 RES 0, L
/* Reset bit 0 in register L to 0. */
IMPL_INSTR(res_0_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB8F RES 1, A
/* Reset bit 1 in register A to 0. */
IMPL_INSTR(res_1_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB88 RES 1, B
/* Reset bit 1 in register B to 0. */
IMPL_INSTR(res_1_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB89 RES 1, C
/* Reset bit 1 in register C to 0. */
IMPL_INSTR(res_1_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB8A RES 1, D
/* Reset bit 1 in register D to 0. */
IMPL_INSTR(res_1_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB8B RES 1, E
/* Reset bit 1 in register E to 0. */
IMPL_INSTR(res_1_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB8C RES 1, H
/* Reset bit 1 in register H to 0. */
IMPL_INSTR(res_1_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB8D RES 1, L
/* Reset bit 1 in register L to 0. */
IMPL_INSTR(res_1_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB97 RES 2, A
/* Reset bit 2 in register A to 0. */
IMPL_INSTR(res_2_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB90 RES 2, B
/* Reset bit 2 in register B to 0. */
IMPL_INSTR(res_2_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB91 RES 2, C
/* Reset bit 2 in register C to 0. */
IMPL_INSTR(res_2_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB92 RES 2, D
/* Reset bit 2 in register D to 0. */
IMPL_INSTR(res_2_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB93 RES 2, E
/* Reset bit 2 in register E to 0. */
IMPL_INSTR(res_2_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB94 RES 2, H
/* Reset bit 2 in register H to 0. */
IMPL_INSTR(res_2_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB95 RES 2, L
/* Reset bit 2 in register L to 0. */
IMPL_INSTR(res_2_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB9F RES 3, A
/* Reset bit 3 in register A to 0. */
IMPL_INSTR(res_3_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB98 RES 3, B
/* Reset bit 3 in register B to 0. */
IMPL_INSTR(res_3_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB99 RES 3, C
/* Reset bit 3 in register C to 0. */
IMPL_INSTR(res_3_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB9A RES 3, D
/* Reset bit 3 in register D to 0. */
IMPL_INSTR(res_3_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB9B RES 3, E
/* Reset bit 3 in register E to 0. */
IMPL_INSTR(res_3_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB9C RES 3, H
/* Reset bit 3 in register H to 0. */
IMPL_INSTR(res_3_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB9D RES 3, L
/* Reset bit 3 in register L to 0. */
IMPL_INSTR(res_3_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA7 RES 4, A
/* Reset bit 4 in register A to 0. */
IMPL_INSTR(res_4_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA0 RES 4, B
/* Reset bit 4 in register B to 0. */
IMPL_INSTR(res_4_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA1 RES 4, C
/* Reset bit 4 in register C to 0. */
IMPL_INSTR(res_4_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA2 RES 4, D
/* Reset bit 4 in register D to 0. */
IMPL_INSTR(res_4_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA3 RES 4, E
/* Reset bit 4 in register E to 0. */
IMPL_INSTR(res_4_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA4 RES 4, H
/* Reset bit 4 in register H to 0. */
IMPL_INSTR(res_4_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA5 RES 4, L
/* Reset bit 4 in register L to 0. */
IMPL_INSTR(res_4_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBAF RES 5, A
/* Reset bit 5 in register A to 0. */
IMPL_INSTR(res_5_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA8 RES 5, B
/* Reset bit 5 in register B to 0. */
IMPL_INSTR(res_5_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA9 RES 5, C
/* Reset bit 5 in register C to 0. */
IMPL_INSTR(res_5_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBAA RES 5, D
/* Reset bit 5 in register D to 0. */
IMPL_INSTR(res_5_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBAB RES 5, E
/* Reset bit 5 in register E to 0. */
IMPL_INSTR(res_5_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBAC RES 5, H
/* Reset bit 5 in register H to 0. */
IMPL_INSTR(res_5_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBAD RES 5, L
/* Reset bit 5 in register L to 0. */
IMPL_INSTR(res_5_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB7 RES 6, A
/* Reset bit 6 in register A to 0. */
IMPL_INSTR(res_6_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB0 RES 6, B
/* Reset bit 6 in register B to 0. */
IMPL_INSTR(res_6_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB1 RES 6, C
/* Reset bit 6 in register C to 0. */
IMPL_INSTR(res_6_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB2 RES 6, D
/* Reset bit 6 in register D to 0. */
IMPL_INSTR(res_6_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB3 RES 6, E
/* Reset bit 6 in register E to 0. */
IMPL_INSTR(res_6_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB4 RES 6, H
/* Reset bit 6 in register H to 0. */
IMPL_INSTR(res_6_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB5 RES 6, L
/* Reset bit 6 in register L to 0. */
IMPL_INSTR(res_6_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBBF RES 7, A
/* Reset bit 7 in register A to 0. */
IMPL_INSTR(res_7_a) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB8 RES 7, B
/* Reset bit 7 in register B to 0. */
IMPL_INSTR(res_7_b) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB9 RES 7, C
/* Reset bit 7 in register C to 0. */
IMPL_INSTR(res_7_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBBA RES 7, D
/* Reset bit 7 in register D to 0. */
IMPL_INSTR(res_7_d) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBBB RES 7, E
/* Reset bit 7 in register E to 0. */
IMPL_INSTR(res_7_e) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBBC RES 7, H
/* Reset bit 7 in register H to 0. */
IMPL_INSTR(res_7_h) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBBD RES 7, L
/* Reset bit 7 in register L to 0. */
IMPL_INSTR(res_7_l) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB86 RES 0, (HL)
/* Reset bit 0 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_0__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB8E RES 1, (HL)
/* Reset bit 1 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_1__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB96 RES 2, (HL)
/* Reset bit 2 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_2__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CB9E RES 3, (HL)
/* Reset bit 3 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_3__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBA6 RES 4, (HL)
/* Reset bit 4 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_4__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBAE RES 5, (HL)
/* Reset bit 5 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_5__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBB6 RES 6, (HL)
/* Reset bit 6 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_6__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// CBBE RES 7, (HL)
/* Reset bit 7 in the memory location specified by register pair HL to 0. */
IMPL_INSTR(res_7__hl_) { throw gbemu::gbemu_exception{"not implemented"}; }

// C3 JP a16
/* Load the 16-bit immediate operand a16 into the program counter (PC). a16 specifies the address of the subsequently executed instruction.
The second byte of the object code (immediately following the opcode) corresponds to the lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to the higher-order byte (bits 8-15). */
IMPL_INSTR(jp_a16) { 
	auto nn = cpu.mmu.read_u16(cpu.regs.pc);
	cpu.regs.pc = nn;

	// clock.m(3)
	// clock.t(16)
}

// C2 JP NZ, a16
/* Load the 16-bit immediate operand a16 into the program counter PC if the Z flag is 0. If the Z flag is 0, then the subsequent instruction starts at address a16. If not, the contents of PC are incremented, and the next instruction following the current JP instruction is executed (as usual).
The second byte of the object code (immediately following the opcode) corresponds to the lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to the higher-order byte (bits 8-15). */
IMPL_INSTR(jp_nz_a16) { throw gbemu::gbemu_exception{"not implemented"}; }

// CA JP Z, a16
/* Load the 16-bit immediate operand a16 into the program counter PC if the Z flag is 1. If the Z flag is 1, then the subsequent instruction starts at address a16. If not, the contents of PC are incremented, and the next instruction following the current JP instruction is executed (as usual).
The second byte of the object code (immediately following the opcode) corresponds to the lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to the higher-order byte (bits 8-15). */
IMPL_INSTR(jp_z_a16) { throw gbemu::gbemu_exception{"not implemented"}; }

// D2 JP NC, a16
/* Load the 16-bit immediate operand a16 into the program counter PC if the CY flag is 0. If the CY flag is 0, then the subsequent instruction starts at address a16. If not, the contents of PC are incremented, and the next instruction following the current JP instruction is executed (as usual).
The second byte of the object code (immediately following the opcode) corresponds to the lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to the higher-order byte (bits 8-15). */
IMPL_INSTR(jp_nc_a16) { throw gbemu::gbemu_exception{"not implemented"}; }

// DA JP C, a16
/* Load the 16-bit immediate operand a16 into the program counter PC if the CY flag is 1. If the CY flag is 1, then the subsequent instruction starts at address a16. If not, the contents of PC are incremented, and the next instruction following the current JP instruction is executed (as usual).
The second byte of the object code (immediately following the opcode) corresponds to the lower-order byte of a16 (bits 0-7), and the third byte of the object code corresponds to the higher-order byte (bits 8-15). */
IMPL_INSTR(jp_c_a16) { throw gbemu::gbemu_exception{"not implemented"}; }

// 18 JR s8
/* Jump s8 steps from the current address in the program counter (PC). (Jump relative.) */
IMPL_INSTR(jr_s8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 20 JR NZ, s8
/* If the Z flag is 0, jump s8 steps from the current address stored in the program counter (PC). If not, the instruction following the current JP instruction is executed (as usual). */
IMPL_INSTR(jr_nz_s8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 28 JR Z, s8
/* If the Z flag is 1, jump s8 steps from the current address stored in the program counter (PC). If not, the instruction following the current JP instruction is executed (as usual). */
IMPL_INSTR(jr_z_s8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 30 JR NC, s8
/* If the CY flag is 0, jump s8 steps from the current address stored in the program counter (PC). If not, the instruction following the current JP instruction is executed (as usual). */
IMPL_INSTR(jr_nc_s8) { throw gbemu::gbemu_exception{"not implemented"}; }

// 38 JR C, s8
/* If the CY flag is 1, jump s8 steps from the current address stored in the program counter (PC). If not, the instruction following the current JP instruction is executed (as usual). */
IMPL_INSTR(jr_c_s8) { throw gbemu::gbemu_exception{"not implemented"}; }

// E9 JP HL
/* Load the contents of register pair HL into the program counter PC. The next instruction is fetched from the location specified by the new value of PC. */
IMPL_INSTR(jp_hl) { throw gbemu::gbemu_exception{"not implemented"}; }

// CD CALL a16
/* In memory, push the program counter PC value corresponding to the address following the CALL instruction to the 2 bytes following the byte specified by the current stack pointer SP. Then load the 16-bit immediate operand a16 into PC.
The subroutine is placed after the location specified by the new PC value. When the subroutine finishes, control is returned to the source program using a return instruction and by popping the starting address of the next instruction (which was just pushed) and moving it to the PC.
With the push, the current value of SP is decremented by 1, and the higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then decremented by 1 again, and the lower-order byte of PC is loaded in the memory address specified by that value of SP.
The lower-order byte of a16 is placed in byte 2 of the object code, and the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_a16) { throw gbemu::gbemu_exception{"not implemented"}; }

// C4 CALL NZ, a16
/* If the Z flag is 0, the program counter PC value corresponding to the memory location of the instruction following the CALL instruction is pushed to the 2 bytes following the memory byte specified by the stack pointer SP. The 16-bit immediate operand a16 is then loaded into PC.
The lower-order byte of a16 is placed in byte 2 of the object code, and the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_nz_a16) { throw gbemu::gbemu_exception{"not implemented"}; }

// CC CALL Z, a16
/* If the Z flag is 1, the program counter PC value corresponding to the memory location of the instruction following the CALL instruction is pushed to the 2 bytes following the memory byte specified by the stack pointer SP. The 16-bit immediate operand a16 is then loaded into PC.
The lower-order byte of a16 is placed in byte 2 of the object code, and the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_z_a16) { throw gbemu::gbemu_exception{"not implemented"}; }

// D4 CALL NC, a16
/* If the CY flag is 0, the program counter PC value corresponding to the memory location of the instruction following the CALL instruction is pushed to the 2 bytes following the memory byte specified by the stack pointer SP. The 16-bit immediate operand a16 is then loaded into PC.
The lower-order byte of a16 is placed in byte 2 of the object code, and the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_nc_a16) { throw gbemu::gbemu_exception{"not implemented"}; }

// DC CALL C, a16
/* If the CY flag is 1, the program counter PC value corresponding to the memory location of the instruction following the CALL instruction is pushed to the 2 bytes following the memory byte specified by the stack pointer SP. The 16-bit immediate operand a16 is then loaded into PC.
The lower-order byte of a16 is placed in byte 2 of the object code, and the higher-order byte is placed in byte 3. */
IMPL_INSTR(call_c_a16) { throw gbemu::gbemu_exception{"not implemented"}; }

// C9 RET
/* Pop from the memory stack the program counter PC value pushed when the subroutine was called, returning contorl to the source program.
The contents of the address specified by the stack pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched from the address specified by the content of PC (as usual). */
IMPL_INSTR(ret) { throw gbemu::gbemu_exception{"not implemented"}; }

// D9 RETI
/* Used when an interrupt-service routine finishes. The address for the return from the interrupt is loaded in the program counter PC. The master interrupt enable flag is returned to its pre-interrupt status.
The contents of the address specified by the stack pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched from the address specified by the content of PC (as usual). */
IMPL_INSTR(reti) { throw gbemu::gbemu_exception{"not implemented"}; }

// C0 RET NZ
/* If the Z flag is 0, control is returned to the source program by popping from the memory stack the program counter PC value that was pushed to the stack when the subroutine was called.
The contents of the address specified by the stack pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched from the address specified by the content of PC (as usual). */
IMPL_INSTR(ret_nz) { throw gbemu::gbemu_exception{"not implemented"}; }

// C8 RET Z
/* If the Z flag is 1, control is returned to the source program by popping from the memory stack the program counter PC value that was pushed to the stack when the subroutine was called.
The contents of the address specified by the stack pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched from the address specified by the content of PC (as usual). */
IMPL_INSTR(ret_z) { throw gbemu::gbemu_exception{"not implemented"}; }

// D0 RET NC
/* If the CY flag is 0, control is returned to the source program by popping from the memory stack the program counter PC value that was pushed to the stack when the subroutine was called.
The contents of the address specified by the stack pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched from the address specified by the content of PC (as usual). */
IMPL_INSTR(ret_nc) { throw gbemu::gbemu_exception{"not implemented"}; }

// D8 RET C
/* If the CY flag is 1, control is returned to the source program by popping from the memory stack the program counter PC value that was pushed to the stack when the subroutine was called.
The contents of the address specified by the stack pointer SP are loaded in the lower-order byte of PC, and the contents of SP are incremented by 1. The contents of the address specified by the new SP value are then loaded in the higher-order byte of PC, and the contents of SP are incremented by 1 again. (THe value of SP is 2 larger than before instruction execution.) The next instruction is fetched from the address specified by the content of PC (as usual). */
IMPL_INSTR(ret_c) { throw gbemu::gbemu_exception{"not implemented"}; }

// C7 RST 0
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 1th byte of page 0 memory addresses, 0x00. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order byte of the PC is loaded in the memory address specified by that value of SP.
The RST instruction can be used to jump to 1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the PC, and 0x00 is loaded in the lower-order byte. */
IMPL_INSTR(rst_0) { throw gbemu::gbemu_exception{"not implemented"}; }

// CF RST 1
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 2th byte of page 0 memory addresses, 0x08. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order byte of the PC is loaded in the memory address specified by that value of SP.
The RST instruction can be used to jump to 1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the PC, and 0x08 is loaded in the lower-order byte. */
IMPL_INSTR(rst_1) { throw gbemu::gbemu_exception{"not implemented"}; }

// D7 RST 2
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 3th byte of page 0 memory addresses, 0x10. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order byte of the PC is loaded in the memory address specified by that value of SP.
The RST instruction can be used to jump to 1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the PC, and 0x10 is loaded in the lower-order byte. */
IMPL_INSTR(rst_2) { throw gbemu::gbemu_exception{"not implemented"}; }

// DF RST 3
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 4th byte of page 0 memory addresses, 0x18. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order byte of the PC is loaded in the memory address specified by that value of SP.
The RST instruction can be used to jump to 1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the PC, and 0x18 is loaded in the lower-order byte. */
IMPL_INSTR(rst_3) { throw gbemu::gbemu_exception{"not implemented"}; }

// E7 RST 4
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 5th byte of page 0 memory addresses, 0x20. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order byte of the PC is loaded in the memory address specified by that value of SP.
The RST instruction can be used to jump to 1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the PC, and 0x20 is loaded in the lower-order byte. */
IMPL_INSTR(rst_4) { throw gbemu::gbemu_exception{"not implemented"}; }

// EF RST 5
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 6th byte of page 0 memory addresses, 0x28. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order byte of the PC is loaded in the memory address specified by that value of SP.
The RST instruction can be used to jump to 1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the PC, and 0x28 is loaded in the lower-order byte. */
IMPL_INSTR(rst_5) { throw gbemu::gbemu_exception{"not implemented"}; }

// F7 RST 6
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 7th byte of page 0 memory addresses, 0x30. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order byte of the PC is loaded in the memory address specified by that value of SP.
The RST instruction can be used to jump to 1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the PC, and 0x30 is loaded in the lower-order byte. */
IMPL_INSTR(rst_6) { throw gbemu::gbemu_exception{"not implemented"}; }

// FF RST 7
/* Push the current value of the program counter PC onto the memory stack, and load into PC the 8th byte of page 0 memory addresses, 0x38. The next instruction is fetched from the address specified by the new content of PC (as usual).
With the push, the contents of the stack pointer SP are decremented by 1, and the higher-order byte of PC is loaded in the memory address specified by the new SP value. The value of SP is then again decremented by 1, and the lower-order byte of the PC is loaded in the memory address specified by that value of SP.
The RST instruction can be used to jump to 1 of 8 addresses. Because all ofthe addresses are held in page 0 memory, 0x00 is loaded in the higher-orderbyte of the PC, and 0x38 is loaded in the lower-order byte. */
IMPL_INSTR(rst_7) { throw gbemu::gbemu_exception{"not implemented"}; }

// 27 DAA
/* Adjust the accumulator (register A) too a binary-coded decimal (BCD) number after BCD addition and subtraction operations. */
IMPL_INSTR(daa) { throw gbemu::gbemu_exception{"not implemented"}; }

// 2F CPL
/* Take the one's complement (i.e., flip all bits) of the contents of register A. */
IMPL_INSTR(cpl) { throw gbemu::gbemu_exception{"not implemented"}; }

// 00 NOP
/* Only advances the program counter by 1. Performs no other operations that would have an effect. */
IMPL_INSTR(nop) { }

// 3F CCF
/* Flip the carry flag CY. */
IMPL_INSTR(ccf) { throw gbemu::gbemu_exception{"not implemented"}; }

// 37 SCF
/* Set the carry flag CY. */
IMPL_INSTR(scf) { throw gbemu::gbemu_exception{"not implemented"}; }

// F3 DI
/* Reset the interrupt master enable (IME) flag and prohibit maskable interrupts.
Even if a DI instruction is executed in an interrupt routine, the IME flag is set if a return is performed with a RETI instruction. */
IMPL_INSTR(di) { 
	cpu.interrupt_enabled = false;
}

// FB EI
/* Set the interrupt master enable (IME) flag and enable maskable interrupts. This instruction can be used in an interrupt routine to enable higher-order interrupts.
The IME flag is reset immediately after an interrupt occurs. The IME flag reset remains in effect if coontrol is returned from the interrupt routine by a RET instruction. However, if an EI instruction is executed in the interrupt routine, control is returned with IME = 1. */
IMPL_INSTR(ei) { throw gbemu::gbemu_exception{"not implemented"}; }

// 76 HALT
/* After a HALT instruction is executed, the system clock is stopped and HALT mode is entered. Although the system clock is stopped in this status, the oscillator circuit and LCD controller continue to operate.
In addition, the status of the internal RAM register ports remains unchanged.
HALT mode is cancelled by an interrupt or reset signal.
The program counter is halted at the step after the HALT instruction. If both the interrupt request flag and the corresponding interrupt enable flag are set, HALT mode is exited, even if the interrupt master enable flag is not set.
Once HALT mode is cancelled, the program starts from the address indicated by the program counter.
If the interrupt master enable flag is set, the contents of the program coounter are pushed to the stack and control jumps to the starting address of the interrupt.
If the RESET terminal goes LOW in HALT moode, the mode becomes that of a normal reset. */
IMPL_INSTR(halt) { throw gbemu::gbemu_exception{"not implemented"}; }

// 10 STOP
/* Execution of a STOP instruction stops both the system clock and oscillator circuit. STOP mode is entered and the LCD controller also stops. However, the status of the internal RAM register ports remains unchanged.
STOP mode can be cancelled by a reset signal.
If the RESET terminal goes LOW in STOP mode, it becomes that of a normal reset status.
The following conditions should be met before a STOP instruction is executed and stop mode is entered:
All interrupt-enable (IE) flags are reset.
Input to P10-P13 is LOW for all. */
IMPL_INSTR(stop) { throw gbemu::gbemu_exception{"not implemented"}; }

