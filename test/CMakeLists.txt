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
gbemu_add_rom_test(blargg_cpu_instrs
    SCRIPT  blargg_serial_pass.dbg
    ROM     "blargg/cpu_instrs/cpu_instrs.gb"
    EXPECT  "Passed"
    TIMEOUT 120)

gbemu_add_rom_test(blargg_instr_timing
    SCRIPT  blargg_serial_pass.dbg
    ROM     "blargg/instr_timing/instr_timing.gb"
    EXPECT  "Passed")

gbemu_add_rom_test(blargg_halt_bug
    SCRIPT  blargg_serial_pass.dbg
    ROM     "blargg/halt_bug.gb"
    EXPECT  "Passed")

gbemu_add_rom_test(blargg_interrupt_time
    SCRIPT  blargg_serial_pass.dbg
    ROM     "blargg/interrupt_time/interrupt_time.gb"
    EXPECT  "Passed")

# Blargg — expected fail until TODO §6 (sub-instruction timing).
# Registered with WILL_FAIL so a future fix flips the test green
# automatically and we get a regression signal.
gbemu_add_rom_test(blargg_mem_timing
    SCRIPT  blargg_serial_pass.dbg
    ROM     "blargg/mem_timing/mem_timing.gb"
    EXPECT  "Passed"
    WILL_FAIL)
