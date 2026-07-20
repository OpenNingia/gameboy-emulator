# GbEmu test target.
#
# Drives the GbEmu executable in --headless mode through a .dbg script
# and matches the captured --out file against an expected regex. See
# TEST_SUITE.md (repo root) for the public ROM suites this is meant to
# consume.
#
# The ROM directory is supplied by the user via a cache variable so the
# repo stays free of binary ROM blobs:
#
#   cmake --preset "default (Debug)" -DGBEMU_TEST_ROMS_DIR=/path/to/roms
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# When GBEMU_TEST_ROMS_DIR is unset, every gbemu_add_rom_test() call
# silently no-ops — CTest then exits clean with zero tests rather than
# failing every CI run that doesn't have the test ROMs available.

set(GBEMU_TEST_ROMS_DIR "" CACHE PATH "Root directory containing public GB/GBC test ROM suites (see TEST_SUITE.md)")

if(NOT GBEMU_TEST_ROMS_DIR)
    message(STATUS "GBEMU_TEST_ROMS_DIR not set; skipping ROM-driven tests. Set it to enable.")
endif()

# gbemu_add_rom_test — register one ROM-driven CTest case.
#
#   gbemu_add_rom_test(<name>
#       SCRIPT  <relative path under test/scripts/>
#       ROM     <relative path under ${GBEMU_TEST_ROMS_DIR}>
#       EXPECT  <regex expected to appear somewhere in the --out file>
#       [TIMEOUT <seconds, default 60>]
#       [WILL_FAIL])
#
# Each test invokes run_test.cmake, which launches the emulator headless
# and then file(READ)s the output and matches EXPECT.  WILL_FAIL flips
# the polarity (used for tests we know we still fail — keeps them in
# the harness so regressions in the *opposite* direction are caught).
function(gbemu_add_rom_test name)
    cmake_parse_arguments(GAT "WILL_FAIL" "SCRIPT;ROM;EXPECT;TIMEOUT" "" ${ARGN})
    if(NOT GAT_SCRIPT OR NOT GAT_ROM OR NOT GAT_EXPECT)
        message(FATAL_ERROR "gbemu_add_rom_test(${name}): SCRIPT, ROM, EXPECT are required")
    endif()
    if(NOT GBEMU_TEST_ROMS_DIR)
        return()
    endif()

    set(_rom "${GBEMU_TEST_ROMS_DIR}/${GAT_ROM}")
    if(NOT EXISTS "${_rom}")
        message(STATUS "  test ${name}: ROM not found (${_rom}); skipping")
        return()
    endif()

    set(_script "${CMAKE_CURRENT_SOURCE_DIR}/scripts/${GAT_SCRIPT}")
    set(_out "${CMAKE_CURRENT_BINARY_DIR}/${name}.out")

    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DGBEMU=$<TARGET_FILE:GbEmu>
            -DSCRIPT=${_script}
            -DROM=${_rom}
            -DOUT=${_out}
            -DEXPECT=${GAT_EXPECT}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake)

    if(GAT_TIMEOUT)
        set_tests_properties(${name} PROPERTIES TIMEOUT ${GAT_TIMEOUT})
    else()
        set_tests_properties(${name} PROPERTIES TIMEOUT 60)
    endif()
    if(GAT_WILL_FAIL)
        set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
    endif()
endfunction()

# ---------------------------------------------------------------
# Test cases.  Paths under GBEMU_TEST_ROMS_DIR follow the layout of
# the c-sp/gameboy-test-roms aggregator zip — adjust if you use a
# different folder structure.
# ---------------------------------------------------------------

# Blargg — expected pass.
#
# All Blargg ROMs paint "Passed" / "Failed" to the LCD via shell.inc;
# halt_bug and interrupt_time do *only* that (no serial output), so we
# route the whole suite through the screen-text matcher for uniformity.
# See test/scripts/blargg_screen_pass.dbg for the script body.
gbemu_add_rom_test(blargg_cpu_instrs
    SCRIPT  blargg_screen_pass.dbg
    ROM     "blargg/cpu_instrs/cpu_instrs.gb"
    EXPECT  "Passed"
    TIMEOUT 120)

gbemu_add_rom_test(blargg_instr_timing
    SCRIPT  blargg_screen_pass.dbg
    ROM     "blargg/instr_timing/instr_timing.gb"
    EXPECT  "Passed")

gbemu_add_rom_test(blargg_halt_bug
    SCRIPT  blargg_screen_pass.dbg
    ROM     "blargg/halt_bug.gb"
    EXPECT  "Passed")

gbemu_add_rom_test(blargg_interrupt_time
    SCRIPT  blargg_screen_pass.dbg
    ROM     "blargg/interrupt_time/interrupt_time.gb"
    EXPECT  "Passed"
    TIMEOUT 120)

gbemu_add_rom_test(blargg_mem_timing
    SCRIPT  blargg_screen_pass.dbg
    ROM     "blargg/mem_timing/mem_timing.gb"
    EXPECT  "Passed")

gbemu_add_rom_test(blargg_mem_timing_2
    SCRIPT  blargg_screen_pass.dbg
    ROM     "blargg/mem_timing-2/mem_timing.gb"
    EXPECT  "Passed")

# Blargg — known-failing rollups.  Registered without WILL_FAIL so the
# real outcome (Fail today) shows up in the CTest summary; flip them to
# WILL_FAIL once we're sure the failure mode is stable and just need a
# regression alarm in the *opposite* direction (a surprise pass).
#
# - oam_bug   : DMG OAM-corruption hardware bug we don't model (TODO §7)
# - dmg_sound : DMG APU correctness (we ship a partial APU)
# - cgb_sound : CGB APU correctness (subset of DMG plus CGB extensions)
gbemu_add_rom_test(blargg_oam_bug
    SCRIPT  blargg_screen_pass.dbg
    ROM     "blargg/oam_bug/oam_bug.gb"
    EXPECT  "Passed"
    TIMEOUT 120)

gbemu_add_rom_test(blargg_dmg_sound
    SCRIPT  blargg_screen_pass.dbg
    ROM     "blargg/dmg_sound/dmg_sound.gb"
    EXPECT  "Passed"
    TIMEOUT 120)

gbemu_add_rom_test(blargg_cgb_sound
    SCRIPT  blargg_screen_pass.dbg
    ROM     "blargg/cgb_sound/cgb_sound.gb"
    EXPECT  "Passed"
    TIMEOUT 120)

# ---------------------------------------------------------------
# Mooneye test suite.
#
# Each Mooneye ROM ends by executing `LD B, B` (opcode $40) as a
# "test complete" marker.  On pass the registers carry the Fibonacci
# fingerprint B=3 C=5 D=8 E=13 H=21 L=34, which dump_regs prints as
# "BC=0305 DE=080D HL=1522" — the EXPECT regex below.  Fails land on
# the same marker with different register values and miss the regex.
# See test/scripts/mooneye_pass.dbg and
# <mooneye-test-suite/game-boy-test-roms-howto.md>.
#
# Tests are registered without WILL_FAIL: model-specific variants
# (-dmg0, -sgb, -GS etc.) intentionally only pass on the exact CPU
# rev they were built for, so the suite necessarily has a baseline
# of "fails by design".  The CTest summary is the snapshot of what
# our DMG-A/B/C/MGB-like core actually achieves.
# ---------------------------------------------------------------

# gbemu_add_mooneye_dir — register every *.gb under a Mooneye subdir.
# The test name is `${prefix}_<flattened relative path without .gb>`,
# with `/` and `-` collapsed to `_` so `ctest -R mooneye_acceptance_ppu`
# selects a feature group cleanly.
function(gbemu_add_mooneye_dir subdir prefix)
    if(NOT GBEMU_TEST_ROMS_DIR)
        return()
    endif()
    set(_root "${GBEMU_TEST_ROMS_DIR}/mooneye-test-suite/${subdir}")
    if(NOT EXISTS "${_root}")
        message(STATUS "  mooneye/${subdir}: not found at ${_root}; skipping")
        return()
    endif()
    file(GLOB_RECURSE _roms RELATIVE "${_root}" "${_root}/*.gb")
    foreach(_rom ${_roms})
        get_filename_component(_dir ${_rom} DIRECTORY)
        get_filename_component(_stem ${_rom} NAME_WE)
        if(_dir STREQUAL "")
            set(_test_name "${prefix}_${_stem}")
        else()
            string(REPLACE "/" "_" _dir_flat "${_dir}")
            set(_test_name "${prefix}_${_dir_flat}_${_stem}")
        endif()
        string(REPLACE "-" "_" _test_name "${_test_name}")
        gbemu_add_rom_test(${_test_name}
            SCRIPT  mooneye_pass.dbg
            ROM     "mooneye-test-suite/${subdir}/${_rom}"
            EXPECT  "BC=0305 DE=080D HL=1522"
            TIMEOUT 30)
    endforeach()
endfunction()

gbemu_add_mooneye_dir("acceptance"     "mooneye_acceptance")
gbemu_add_mooneye_dir("emulator-only"  "mooneye_emu")
gbemu_add_mooneye_dir("misc"           "mooneye_misc")
