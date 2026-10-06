cmake_minimum_required(VERSION 3.29)

project(GbEmu)

add_subdirectory(src)

# CTest integration.  Enabled unconditionally so `ctest` always works;
# the ROM-driven test cases under test/ skip themselves when
# GBEMU_TEST_ROMS_DIR is unset.  See TEST_SUITE.md for the workflow.
enable_testing()
add_subdirectory(test)