# Test harness invoked by every gbemu_add_rom_test() entry in
# test/CMakeLists.txt.  Runs the emulator headless against a script,
# then matches a regex against the captured output file.
#
# Required cache variables (passed via -D from add_test):
#   GBEMU   — absolute path to the GbEmu executable
#   SCRIPT  — absolute path to the .dbg script
#   ROM     — absolute path to the ROM file
#   OUT     — absolute path for the --out capture (overwritten)
#   EXPECT  — regex that must match somewhere in OUT for the test to pass

foreach(var GBEMU SCRIPT ROM OUT EXPECT)
    if(NOT ${var})
        message(FATAL_ERROR "run_test.cmake: ${var} not set")
    endif()
endforeach()

# Wipe any stale capture from the previous run so a hung emulator that
# exits before writing doesn't accidentally satisfy EXPECT.
file(REMOVE "${OUT}")

execute_process(
    COMMAND "${GBEMU}" --headless --script "${SCRIPT}" --out "${OUT}" "${ROM}"
    RESULT_VARIABLE rv
    OUTPUT_VARIABLE stdout_capture
    ERROR_VARIABLE  stderr_capture)

if(NOT rv EQUAL 0)
    message("--- GbEmu stdout ---\n${stdout_capture}")
    message("--- GbEmu stderr ---\n${stderr_capture}")
    message(FATAL_ERROR "GbEmu exited with status ${rv}")
endif()

if(NOT EXISTS "${OUT}")
    message(FATAL_ERROR "GbEmu did not produce output file ${OUT}")
endif()

file(READ "${OUT}" capture)
if(NOT capture MATCHES "${EXPECT}")
    message("--- captured output (${OUT}) ---")
    message("${capture}")
    message("--- end ---")
    message(FATAL_ERROR "Expected regex '${EXPECT}' did not match captured output")
endif()
