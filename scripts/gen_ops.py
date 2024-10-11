import json

def make_cn(text):
	return text.replace(' ', '_').replace(',', '').replace('(', '_').replace(')', '_').replace('[', '_').replace(']', '_').replace('+', 'P').replace('-', 'M').lower()

def main():
	data = ''
	with open('ops_db.json', 'rt') as fp:
		data = json.load(fp)

	with open('opcodes.hpp', 'wt') as hdr:
		with open('opcodes.cpp', 'wt') as cpp:
		
			for f in [hdr, cpp]:
				f.write('// Generated from ops_db.json\n')
				f.write('// https://gist.github.com/bberak/ca001281bb8431d2706afd31401e802b\n')
				f.write('\n')

			hdr.write('#pragma once\n')
			hdr.write('#ifndef _H_OPCODES_H_\n')
			hdr.write('#define _H_OPCODES_H_\n')

			hdr.write('#include <cpu.h>\n')
			hdr.write('#include <unordered_map>\n')
			hdr.write(
			"""
#define DEF_INSTR( o, x, y ) \\
	struct x##: instruction { \\
		## x ##() : instruction(y) { instruction_set.insert({o, *this}); } \\
		void execute(cpu& cpu) override; };

#define INST_INSTR(x) extern gbemu::instruction_types::##x x##_;
			""")

			hdr.write(
			"""
namespace gbemu {
	extern std::unordered_map<std::uint16_t, instruction&> instruction_set;
	namespace instruction_types {
			""")

			for op in data:
				mnemonic = op['mnemonic']
				cn = make_cn(mnemonic)
				opcode = '0x00' + op['opCode'] if len(op['opCode']) == 2 else '0x' + op['opCode']
				hdr.write(
				f"""
		DEF_INSTR({opcode}, {cn}, "{mnemonic}");""")

			hdr.write(
			"""
	}
	
	namespace instructions {
			""")

			for op in data:
				mnemonic = op['mnemonic']
				cn = make_cn(mnemonic)

				hdr.write(
				f"""
		INST_INSTR({cn});"""
				)


			hdr.write(
			"""
	}
}
"""
			)

			hdr.write('#endif /* _H_OPCODES_H_ */')

			cpp.write('#include <opcodes.hpp>\n')
			cpp.write('#include <exc.hpp>\n')
			cpp.write(
			"""
#define IMPL_INSTR(x) \\
	gbemu::instruction_types::##x gbemu::instructions::x##_{}; \\
	void gbemu::instruction_types::##x##::execute(cpu& cpu)

std::unordered_map<std::uint16_t, gbemu::instruction&> gbemu::instruction_set;

"""
			)

			for op in data:
				mnemonic = op['mnemonic']
				description = op['description']
				cn = make_cn(mnemonic)
				opcode = op['opCode']


				cpp.write(f'// {opcode} {mnemonic}\n')
				cpp.write(f'/* {description} */\n')
				cpp.write(f'IMPL_INSTR({cn}) {{ throw gbemu::gbemu_exception{{"not implemented"}}; }}\n')
				cpp.write('\n')

if __name__ == '__main__':
	main()