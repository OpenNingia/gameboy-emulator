"""Generate inc/opcodes.hpp from scripts/opcodes.json (gbdev.io Opcodes.json).

Source: https://gbdev.io/gb-opcodes/Opcodes.json — the upstream Pan Docs DB,
newer than the previous ops_db.json. Cycles are already T-cycles (no x4
conversion needed), the file includes ILLEGAL_xx entries, and the operand
schema is structured (no string parsing).

The JSON has two top-level dicts ("unprefixed" and "cbprefixed"), each
keyed by "0xNN". Each entry has:
  - mnemonic: just the verb (e.g. "LD", "JR")
  - bytes: instruction size
  - cycles: array — [n] for unconditional, [taken, not_taken] for conditional
  - operands: list of {name, immediate, [bytes|increment|decrement]}
  - flags: Z/N/H/C status

By default the generator emits identifiers compatible with the previous
ops_db.json scheme so existing hand-written instruction bodies in
src/opcodes.cpp keep linking. The legacy transformations applied to
gbdev data are:

  - n8/n16/e8 operand names are rewritten to d8/d16/s8
  - RST targets become rst_0..rst_7 (numeric index) instead of rst_00h..rst_38h
  - STOP drops its n8 operand (was "STOP", now "STOP n8" upstream)
  - SUB/AND/OR/XOR/CP drop the explicit "A" first operand (the old DB
    relied on the implicit accumulator; ADD/ADC/SBC kept it in both schemes)
  - LDH is rewritten to LD (0xE0/0xE2/0xF0/0xF2 high-page accesses)
  - The standalone 0xCB PREFIX entry is skipped (it's dispatch glue, not
    an instruction)
  - ILLEGAL_xx entries are skipped

Pass --gbdev-names to disable all of the above and emit the gbdev names
verbatim. Existing C++ stubs in src/opcodes.cpp will then need to be
renamed (or regenerated).

The emitted DEF_INSTR macro carries T-cycle metadata (cycles_not_taken,
cycles_taken). Conditional opcodes (JR cc / JP cc / CALL cc / RET cc)
encode their cycle count as a (not_taken, taken) pair; everything else
has both fields equal.

Usage:
    python gen_ops.py                         # writes ./opcodes.hpp from ./opcodes.json
    python gen_ops.py --out ../inc/opcodes.hpp
    python gen_ops.py --db opcodes.json --out -   # '-' prints to stdout
    python gen_ops.py --gbdev-names               # use upstream operand names
    python gen_ops.py --include-illegal           # emit ILLEGAL_xx entries
"""

import argparse
import json
import sys
from pathlib import Path


# Map gbdev operand names to the legacy convention used by existing
# hand-written instruction bodies in src/opcodes.cpp.
LEGACY_OPERAND_RENAME = {
    'n8': 'd8',
    'n16': 'd16',
    'e8': 's8',
}

# Verbs where the old DB elided the implicit A accumulator operand. The
# gbdev DB always lists A explicitly; legacy mode drops it for these.
# ADD/ADC/SBC are deliberately excluded — they kept A in both schemes
# because non-A variants exist (e.g. ADD HL, BC).
LEGACY_DROP_LEADING_A_VERBS = {'SUB', 'AND', 'OR', 'XOR', 'CP'}


def make_cn(text: str) -> str:
    """Convert a mnemonic into a valid C++ identifier."""
    return (text
            .replace(' ', '_')
            .replace(',', '')
            .replace('(', '_').replace(')', '_')
            .replace('[', '_').replace(']', '_')
            .replace('+', 'P').replace('-', 'M')
            .replace('$', '')
            .lower())


def rename_operand(name: str, legacy: bool) -> str:
    """Rewrite an operand name. In legacy mode applies the n8/n16/e8 -> d8/d16/s8
    rename. RST targets ($NN) become a single digit in legacy mode and "NNH" in
    gbdev mode (the dollar sign would be invalid in a C++ identifier either way)."""
    if name.startswith('$'):
        if legacy:
            # $00, $08, $10, ..., $38 -> 0..7
            return str(int(name[1:], 16) // 8)
        return name[1:] + 'H'
    if legacy:
        return LEGACY_OPERAND_RENAME.get(name, name)
    return name


def render_operand(op: dict, legacy: bool) -> str:
    name = rename_operand(op['name'], legacy)
    if op.get('increment'):
        name = name + '+'
    elif op.get('decrement'):
        name = name + '-'
    if not op.get('immediate', True):
        name = f'({name})'
    return name


def build_mnemonic(entry: dict, legacy: bool) -> str:
    """Reconstruct the textual mnemonic like 'LD A, (HL+)' from operands."""
    verb = entry['mnemonic']
    operands = entry.get('operands', [])

    if legacy:
        # gbdev models the STOP trailing pad byte as an explicit n8 operand;
        # the old DB had verb-only "STOP".
        if verb == 'STOP' and len(operands) == 1 and operands[0]['name'] == 'n8':
            return verb
        # SUB/AND/OR/XOR/CP: drop the explicit A first operand to match old DB.
        if (verb in LEGACY_DROP_LEADING_A_VERBS
                and operands
                and operands[0]['name'] == 'A'
                and operands[0].get('immediate', True)):
            operands = operands[1:]
        # LDH was just called LD in the old DB.
        if verb == 'LDH':
            verb = 'LD'

    parts: list[str] = []
    i = 0
    while i < len(operands):
        op = operands[i]
        # 0xF8 "LD HL, SP+e8" — the SP operand has immediate=True + increment=True,
        # and the next operand is the signed offset that gets added in. Render
        # the pair fused as "SP+e8" rather than as two comma-separated operands.
        if (op.get('increment')
                and op.get('immediate', True)
                and i + 1 < len(operands)):
            base = rename_operand(op['name'], legacy)
            nxt = rename_operand(operands[i + 1]['name'], legacy)
            parts.append(f'{base}+{nxt}')
            i += 2
            continue
        parts.append(render_operand(op, legacy))
        i += 1

    if not parts:
        return verb
    return verb + ' ' + ', '.join(parts)


def parse_cycles(cycles: list) -> tuple[int, int]:
    """Parse the gbdev 'cycles' array (T-cycles). [c] -> (c, c);
    [taken, not_taken] -> (not_taken, taken) to match the DEF_INSTR order."""
    if len(cycles) == 1:
        return (cycles[0], cycles[0])
    taken, not_taken = cycles
    return (not_taken, taken)


def is_illegal(entry: dict) -> bool:
    return entry['mnemonic'].startswith('ILLEGAL_')


def is_prefix(entry: dict) -> bool:
    return entry['mnemonic'] == 'PREFIX'


HEADER_PREAMBLE = """// Generated from scripts/opcodes.json (https://gbdev.io/gb-opcodes/Opcodes.json).
//
// DO NOT EDIT THIS FILE BY HAND. Regenerate via scripts/gen_ops.py.
// Hand-written instruction bodies live in src/opcodes.cpp and are NOT
// touched by the generator.

// clang-format off

#pragma once
#ifndef _H_OPCODES_H_
#define _H_OPCODES_H_

#include <array>
#include <cstdint>
#include <cpu.h>

// DEF_INSTR(opcode, struct_name, mnemonic, cycles_not_taken, cycles_taken)
//
// For unconditional opcodes cycles_not_taken == cycles_taken. For
// conditional branches (JR cc / JP cc / CALL cc / RET cc) the two values
// differ; the instruction body is expected to bump cpu.extra_cycles when
// the branch is taken (see src/opcodes.cpp).
//
// The cycle counts are passed to the instruction base ctor; the user is
// responsible for keeping inc/cpu.h's `instruction` constructor signature
// in sync: instruction(std::string mnemonic, std::uint8_t cycles,
//                      std::uint8_t cycles_taken).
#define DEF_INSTR( o, x, y, c, ct ) \\
\tstruct x: instruction { \\
\t\tx() : instruction(y, c, ct) { \\
\t\t\tif constexpr (((o) & 0xFF00) == 0xCB00) \\
\t\t\t\tinstruction_set_cb[(o) & 0xFF] = this; \\
\t\t\telse \\
\t\t\t\tinstruction_set[(o) & 0xFF] = this; \\
\t\t} \\
\t\tvoid execute(cpu& cpu) override; \\
\t};

#define INST_INSTR(x) extern gbemu::instruction_types::x x##_;

namespace gbemu {
\textern std::array<instruction*, 256> instruction_set;
\textern std::array<instruction*, 256> instruction_set_cb;

\tnamespace instruction_types {
"""

HEADER_MID = """\t}

\tnamespace instructions {
"""

HEADER_EPILOGUE = """\t}
}

#endif /* _H_OPCODES_H_ */
"""


def collect(db: dict, *, legacy: bool, include_illegal: bool):
    """Walk the gbdev DB into a flat ordered list of
    (opcode, identifier, mnemonic, cycles_not_taken, cycles_taken)."""
    rows = []
    for prefix_name, prefix_high in (('unprefixed', 0x0000), ('cbprefixed', 0xCB00)):
        block = db.get(prefix_name, {})
        for key in sorted(block.keys(), key=lambda s: int(s, 16)):
            entry = block[key]
            if not include_illegal and is_illegal(entry):
                continue
            # The standalone CB PREFIX entry is dispatch glue (the CB-prefixed
            # table handles its successors); legacy mode drops it.
            if legacy and is_prefix(entry):
                continue
            opcode = prefix_high | int(key, 16)
            mnemonic = build_mnemonic(entry, legacy)
            cn = make_cn(mnemonic)
            nt, t = parse_cycles(entry['cycles'])
            rows.append((opcode, cn, mnemonic, nt, t))
    return rows


def emit_header(rows, out) -> None:
    out.write(HEADER_PREAMBLE)
    for opcode, cn, mnemonic, nt, t in rows:
        out.write(f'\t\tDEF_INSTR(0x{opcode:04X}, {cn}, "{mnemonic}", {nt}, {t});\n')
    out.write(HEADER_MID)
    for _opcode, cn, *_ in rows:
        out.write(f'\t\tINST_INSTR({cn});\n')
    out.write(HEADER_EPILOGUE)


def main() -> None:
    here = Path(__file__).parent

    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--db', default=str(here / 'opcodes.json'),
                   help="Path to the gbdev Opcodes.json (default: alongside this script)")
    p.add_argument('--out', default='opcodes.hpp',
                   help="Output path for opcodes.hpp. Use '-' for stdout. "
                        "Default: ./opcodes.hpp (current dir)")
    p.add_argument('--gbdev-names', action='store_true',
                   help="Use gbdev operand naming (n8/n16/e8, RST $NN as NNH) "
                        "instead of the legacy ops_db.json scheme (d8/d16/s8, "
                        "RST 0..7). Breaks identifier compatibility with the "
                        "existing src/opcodes.cpp stubs.")
    p.add_argument('--include-illegal', action='store_true',
                   help="Emit ILLEGAL_xx opcodes (skipped by default for parity "
                        "with the previous ops_db.json generator).")
    args = p.parse_args()

    with open(args.db, 'rt', encoding='utf-8') as fp:
        db = json.load(fp)

    if 'unprefixed' not in db:
        raise SystemExit(f"{args.db} does not look like a gbdev Opcodes.json "
                         "(missing top-level 'unprefixed' key)")

    rows = collect(db, legacy=not args.gbdev_names,
                   include_illegal=args.include_illegal)

    if args.out == '-':
        emit_header(rows, sys.stdout)
    else:
        with open(args.out, 'wt', encoding='utf-8', newline='\n') as fp:
            emit_header(rows, fp)
        print(f"wrote {args.out} ({len(rows)} opcodes)", file=sys.stderr)


if __name__ == '__main__':
    main()
