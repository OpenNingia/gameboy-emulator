#include <fstream>

#include <SDL2/SDL.h>
#include <app.h>
#include <core.h>
#include <exc.hpp>

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

    auto window = SDL_CreateWindow("GbEmu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, cfg.win.width,
                                   cfg.win.height, SDL_WINDOW_SHOWN);

    if (!window)
        throw gbemu::gbemu_exception{"SDL Window creation failed!"};

    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
        throw gbemu::gbemu_exception{"SDL Renderer creation failed!"};

    SDL_RenderSetLogicalSize(renderer, 160, 144);

    // load bios(
    if (!cfg.bios.path.empty()) {
        load_bios(core, cfg.bios.path);
    }

    // load rom
    load_rom(core, cfg.rom.path);

    core.init();

    // const int speed = 16;

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
                // F12: diagnostic dump of the PC ring (useful when chasing
                // ROMs that hang in an infinite loop).
                if (e.key.keysym.sym == SDLK_F12) {
                    core.dump_pc_ring();
                }
                // game.input(e.key.keysym.scancode);
            }
        }

        // 4.19 MHz / 60 fps ≈ 69905 T-cycles per frame
        constexpr std::uint32_t CYCLES_PER_FRAME = 70224; // valore esatto DMG
        std::uint32_t budget = 0;
        while (budget < CYCLES_PER_FRAME) {
            budget += core.step(); // step ora deve ritornare i T-cycle consumati
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
