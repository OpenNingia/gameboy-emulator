# Test ROM suites

Curated list of public Game Boy / Game Boy Color test ROM suites used by
the emulator-dev community. They cover CPU correctness, timing, APU,
PPU, MBC banking, DMA, RTC — i.e. exactly the subsystems where bugs are
silent until a specific game trips on them. Developing **against test
suites** instead of "let me load a random ROM and squint" is the only
sustainable approach beyond the early scaffolding stage.

The repo includes a CTest integration (`test/CMakeLists.txt`) that
drives a headless ROM through a `.dbg` script and matches the output —
see *Workflow* below for the configure flow and how to add a new test.

---

## Aggregator (start here)

- **c-sp/gameboy-test-roms** — <https://github.com/c-sp/gameboy-test-roms>
  Curated mega-collection. Distributes **pre-built** `.gb` / `.gbc`
  binaries of every suite below in versioned release zips
  ([releases](https://github.com/c-sp/gameboy-test-roms/releases)). Much
  faster than cloning ten repos and building each with RGBDS.

---

## CPU / timing / misc

| Suite | Repo | Coverage | Our status |
|-------|------|----------|------------|
| Blargg `cpu_instrs` | <https://github.com/retrio/gb-test-roms> | Every CPU opcode, semantics of flags and operands | Pass |
| Blargg `instr_timing` | same | Per-opcode T-cycle count | Pass |
| Blargg `mem_timing` / `mem_timing-2` | same | Sub-instruction timing of memory bus accesses (mid-opcode read/write cycles) | Pass |
| Blargg `halt_bug` | same | HALT instruction edge case when IME=0 and pending IRQ | Pass |
| Blargg `interrupt_time` | same | Number of cycles consumed by IRQ dispatch | Pass |
| Blargg `oam_bug` | same | DMG-only OAM corruption when inc/dec near sprite RAM | Not implemented (DMG hardware bug we don't model) |
| Mooneye `acceptance/instr/` | <https://github.com/Gekkio/mooneye-test-suite> | Opcode edge cases (DAA, RET on HL, etc.) | Mostly pass |
| Mooneye `acceptance/interrupts/` | same | IRQ priority, dispatch timing, IF/IE behavior | Mixed |
| Mooneye `acceptance/timer/` | same | TIMA/TMA/TAC overflow, reload latency | Mixed |
| Mooneye `acceptance/bits/` | same | Register bit masks (unimplemented bits read as 1) | Pass |
| GBMicrotest | <https://github.com/aappleby/GBMicrotest> | Per-cycle micro-tests, mostly PPU/timer edge cases | Useful only after pixel-FIFO refactor |
| little-things-gb | <https://github.com/pinobatch/little-things-gb> | Pinobatch's targeted regression set | Mixed |

Blargg source tarball (Parodius mirror): <https://gbdev.gg8.se/files/roms/blargg-gb-tests/>

---

## APU / audio

| Suite | Repo | Coverage | Our status |
|-------|------|----------|------------|
| Blargg `dmg_sound` | <https://github.com/retrio/gb-test-roms> | DMG APU — 12 sub-tests (length counter, sweep, envelope, NRx2/NRx4 retrigger, wave RAM, channel mixing) | In progress |
| Blargg `cgb_sound` | same | CGB-specific APU deltas (wave RAM redirect, length/freq edge cases) | In progress (recent fixes for `09:01` and `12:0x` wave RAM redirect) |
| SameSuite `apu/` | <https://github.com/LIJI32/SameSuite> | SameBoy's APU regression tests; covers scenarios Blargg doesn't | Untested |

---

## PPU — visual checkmark

These produce a single framebuffer at a known cycle and compare against
a reference PNG. The CTest target shells out to `dump framebuffer
PATH.ppm` and compares (see *Workflow*).

| Suite | Repo | Coverage | Our status |
|-------|------|----------|------------|
| dmg-acid2 | <https://github.com/mattcurrie/dmg-acid2> | BG / window / sprite priority, 8×16 sprites, BGP/OBP, sprite-X tie-break | Likely fail (sprite edge cases) |
| cgb-acid2 | <https://github.com/mattcurrie/cgb-acid2> | CGB attribute byte (palette, VRAM bank, flip, BG-over-OBJ priority), master priority bit | Likely fail (similar) |
| cgb-acid-hell | <https://github.com/mattcurrie/cgb-acid-hell> | Stress variant of cgb-acid2 | Fail |
| **mealybug-tearoom-tests** ⭐ | <https://github.com/mattcurrie/mealybug-tearoom-tests> | Mid-frame PPU effects: LCDC writes, palette swaps, SCX/SCY changes via STAT IRQ, window restart | **Fail wholesale** — requires pixel-FIFO PPU (TODO §19). Every scanline-based emulator fails the same way. This is the regression suite for §19. |
| Mooneye `acceptance/ppu/` | <https://github.com/Gekkio/mooneye-test-suite> | STAT mode timing, LYC interrupt, vblank interrupt timing, intr_2 chains | Mixed |
| BullyGB | <https://github.com/Hacktix/BullyGB> | STAT IRQ blocking / latching edge case | Untested |
| strikethrough | <https://github.com/Hacktix/strikethrough.gb> | Per-line PPU timing focus | Untested |

---

## MBC / RTC / battery

| Suite | Repo | Coverage | Our status |
|-------|------|----------|------------|
| Mooneye `emulator-only/mbc1/` | <https://github.com/Gekkio/mooneye-test-suite> | MBC1 bank-low, bank-high, mode select, RAM banking, mirroring on 2 MB ROMs | Mixed |
| Mooneye `emulator-only/mbc2/` | same | MBC2 nibble RAM, single-byte bank-low | Untested |
| Mooneye `emulator-only/mbc5/` | same | MBC5 9-bit bank select, RAM banking | **Relevant for Aladdin DX bug** — should run this |
| rtc3test | <https://github.com/aaaaaa123456789/rtc3test> | MBC3 RTC (calendar advance, halt, latch, overflow into day-carry) | Mixed (BESS RTC works on the basics) |

---

## DMA / HDMA

| Suite | Repo | Coverage | Our status |
|-------|------|----------|------------|
| Mooneye `acceptance/oam_dma/` | <https://github.com/Gekkio/mooneye-test-suite> | OAM-DMA timing, source-mirror behavior, conflict with bus accesses | Untested |
| SameSuite `dma/` | <https://github.com/LIJI32/SameSuite> | OAM-DMA + CGB GDMA/HDMA timing | Untested |

---

## Misc curated collections

- **age-test-roms** — <https://github.com/c-sp/age-test-roms> — c-sp's curated set with originals (`m3-*`, `halt-*`) not present elsewhere.
- **AGE emulator** repo — <https://github.com/c-sp/age> — has a few unique tests bundled.

---

## Workflow

### One-time setup

1. Download `gameboy-test-roms-vNN.zip` from the
   [c-sp aggregator releases](https://github.com/c-sp/gameboy-test-roms/releases).
2. Extract somewhere stable, e.g. `C:\gb-test-roms\` or `~/gb-test-roms/`.
   The layout matches each upstream repo (e.g.
   `<root>/blargg/cpu_instrs/cpu_instrs.gb`,
   `<root>/mooneye-test-suite/acceptance/...`).
3. Configure CMake with the path:

   ```powershell
   cmake --preset "default (Debug)" -DGBEMU_TEST_ROMS_DIR="C:/gb-test-roms"
   cmake --build build
   ```

   When `GBEMU_TEST_ROMS_DIR` is unset (the default), the test target
   skips ROM-driven tests at configure time and CTest exits clean
   without errors — useful for environments that don't have the ROMs.

4. Run:

   ```powershell
   ctest --test-dir build --output-on-failure
   ```

### Adding a new test

A test is a script + an expected regex. The script drives the emulator
through a `.dbg` script (same one used by `--headless --script`) and
the regex is matched against the captured `--out` file.

1. Drop the `.dbg` script under `test/scripts/`. Use existing scripts
   as a template — typically:

   ```
   # Blargg-style: success criterion is "Passed" in serial output
   run-until serial-match "Passed" max-cycles 500000000
   serial-dump
   exit
   ```

2. Register the test in `test/CMakeLists.txt`:

   ```cmake
   gbemu_add_rom_test(blargg_cpu_instrs
     SCRIPT  blargg_cpu_instrs.dbg
     ROM     blargg/cpu_instrs/cpu_instrs.gb
     EXPECT  "Passed")
   ```

3. Reconfigure (`cmake --build build`) and `ctest -R blargg_cpu_instrs`
   to run just that one.

### Framebuffer tests (Acid2, Mealybug)

Acid2 and Mealybug compare a *framebuffer* against a reference PNG.
The script runner exposes:

```
dump framebuffer PATH.ppm
```

which writes the current PPU framebuffer as a binary PPM (P6) — 160×144,
8 bits per channel, RGB — at the absolute or output-dir-relative path.

A framebuffer test script typically:

```
run-until vblank max-cycles 30000000   # let the test render
run-until vblank max-cycles 30000000   # one more frame to settle
dump framebuffer out.ppm
exit
```

The CTest harness then post-processes by SHA-256 / pixel-compare against
the reference (currently a manual step — the wrapper script
`test/run_test.cmake` only does regex matching today; PNG/PPM diff is a
TODO once we settle on a comparator library).

### Priority order for fixing failing suites

Given the current state of the project, in cost/value order:

1. **Mooneye `emulator-only/mbc5/`** — relevant to current debugging
   (Aladdin DX, MBC5), small surface, immediate signal.
2. **Blargg `dmg_sound` / `cgb_sound`** — already in progress; finish.
3. **Mooneye `acceptance/ppu/`** — baseline PPU timing; foothold for
   the §19 pixel-FIFO refactor.
4. **dmg-acid2 / cgb-acid2** — visual confirm that BG/window/sprites
   compose correctly; quick to look at and identify what's off.
5. **mealybug-tearoom-tests** — only meaningful after §19; track the
   "fails" count as a refactor metric.
6. **SameSuite apu/** — once the APU stabilises and Blargg passes.
