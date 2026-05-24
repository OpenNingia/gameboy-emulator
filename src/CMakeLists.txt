cmake_minimum_required(VERSION 3.29)

project(GbEmu)

find_package(SDL2 CONFIG REQUIRED)
find_package(LibConfig CONFIG REQUIRED)
find_package(quill CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# WIN32 sets the /SUBSYSTEM:WINDOWS linker flag on MSVC so the executable
# does not open a console window when launched.  On non-Windows platforms
# the keyword is a no-op.  SDL2main provides the WinMain entry point that
# forwards to our standard `int main(int, char**)`.
add_executable(GbEmu WIN32 "gbemu.cpp" "app.cpp" "core.cpp" "cpu.cpp" "irq.cpp" "timer.cpp" "opcodes.cpp" "cfg.cpp" "card.cpp" "mmu.cpp" "mbc.cpp" "ppu.cpp" "serial.cpp" "dma.cpp" "log.cpp" "disasm.cpp" "debugger.cpp" "script_runner.cpp" "ui.cpp" "apu.cpp" "joypad.cpp" "gfx.cpp")
set_property(TARGET GbEmu PROPERTY CXX_STANDARD 20)

target_link_libraries(GbEmu
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    libconfig::libconfig libconfig::libconfig++
    quill::quill
    CLI11::CLI11
    absl::inlined_vector
    imgui::imgui
)

target_include_directories(GbEmu PRIVATE ${CMAKE_SOURCE_DIR}/inc)

configure_file(
    ${CMAKE_SOURCE_DIR}/cfg/gbemu.conf
    ${CMAKE_CURRENT_BINARY_DIR}/cfg/gbemu.conf
    COPYONLY
)