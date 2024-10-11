cmake_minimum_required(VERSION 3.29)

project(GbEmu)

find_package(SDL2 CONFIG REQUIRED)
find_package(LibConfig CONFIG REQUIRED)

add_executable(GbEmu "gbemu.cpp" "app.cpp" "cpu.cpp" "opcodes.cpp" "cfg.cpp" "card.cpp")
set_property(TARGET GbEmu PROPERTY CXX_STANDARD 20)

target_link_libraries(GbEmu
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    libconfig::libconfig libconfig::libconfig++
)

target_include_directories(GbEmu PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_compile_definitions(GbEmu PRIVATE ENTT_STANDARD_CPP)

configure_file(
    ${CMAKE_SOURCE_DIR}/cfg/gbemu.conf
    ${CMAKE_CURRENT_BINARY_DIR}/cfg/gbemu.conf
    COPYONLY
)