#include <cstdio>
#include <fstream>
#include <iostream>

#include <SDL2/SDL.h>
#include <app.h>
#include <core.h>
#include <debugger.h>
#include <exc.hpp>

namespace gbemu {
    // Defined in src/script_runner.cpp.
    int run_script(debugger& dbg, const std::string& script_path, const std::string& out_path);
} // namespace gbemu

Application::Application() : cfg("cfg/gbemu.conf") {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
        throw gbemu::gbemu_exception{"SDL Initialization failed!"};
}

Application::~Application() {
    SDL_Quit();
}

static void load_rom(gbemu::core& core, const std::string& path) {
    gbemu::rom_file c{};
    c.load_from(path);
    core.load(c);
}

static void load_bios(gbemu::core& core, const std::string& path) {
    gbemu::bios_file c{};
    c.load_from(path);
    core.load(c);
}

void Application::set_rom_file(std::string_view path) {
    cfg.rom.path = path;
}

void Application::run() {
    gbemu::core core;
    gbemu::debugger debugger{core};

    // Stdout-echo of bytes the cartridge transmits via the serial link.  The
    // hardware-emulation half of $FF02 (clear SC bit 7, raise IF.3) lives in
    // serial::on_sc_write; this observer is just for human-visible output and
    // will be removed once the PR5 ImGui serial panel ships.  Skipped in
    // headless mode (the debugger's serial ring buffer serves serial-dump).
    if (!headless_) {
        core.mmu.add_mmio_write_handler(0xFF02, [&core](std::uint8_t v) {
            if (v == 0x81) {
                std::putchar(static_cast<char>(core.mmu.hwr_sb()));
                std::fflush(stdout);
            }
        });
    }

    // load bios
    if (!cfg.bios.path.empty()) {
        load_bios(core, cfg.bios.path);
    }

    // load rom
    load_rom(core, cfg.rom.path);

    core.init();

    if (headless_) {
        // No window, no renderer, no event loop — just run the script and
        // return.  PPU keeps running internally; its framebuffer is just
        // never presented, so VBlank edges remain observable by run-until.
        gbemu::run_script(debugger, script_path_, output_path_);
        return;
    }

    auto window = SDL_CreateWindow("GbEmu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, cfg.win.width,
                                   cfg.win.height, SDL_WINDOW_SHOWN);

    if (!window)
        throw gbemu::gbemu_exception{"SDL Window creation failed!"};

    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
        throw gbemu::gbemu_exception{"SDL Renderer creation failed!"};

    SDL_RenderSetLogicalSize(renderer, 160, 144);

    bool quit = false;

    // texture
    auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

    std::array<std::uint32_t, 160 * 144> framebuffer{};

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            } else if (e.type == SDL_KEYDOWN) {
                // F12: diagnostic dump of registers + recent PCs (useful when
                // chasing ROMs that hang in an infinite loop).  Routed through
                // the debugger so the format matches the headless script
                // runner.
                if (e.key.keysym.sym == SDLK_F12) {
                    std::cout << "=== regs @cycle=" << debugger.total_cycles() << " ===\n";
                    debugger.dump_regs(std::cout);
                    std::cout << "=== pc-ring 32 ===\n";
                    debugger.dump_pc_ring(std::cout, 32);
                    std::cout.flush();
                }
                // game.input(e.key.keysym.scancode);
            }
        }

        // 4.19 MHz / 60 fps ≈ 69905 T-cycles per frame
        constexpr std::uint32_t CYCLES_PER_FRAME = 70224; // valore esatto DMG
        std::uint32_t budget = 0;
        while (budget < CYCLES_PER_FRAME) {
            // Route through the debugger seam.  Breakpoints / watchpoints are
            // not enforced in interactive mode here (yet) — the script runner
            // is the only consumer that acts on them in PR2; PR3 wires this
            // up to the ImGui controls.
            budget += debugger.step().cycles;
        }

        if (core.ppu.consume_frame_ready()) {
            SDL_UpdateTexture(texture, nullptr, core.ppu.framebuffer(), 160 * 4);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
