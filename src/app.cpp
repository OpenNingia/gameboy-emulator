#include <app.h>
#include <cpu.h>
#include <exc.hpp>

#include <SDL2/SDL.h>

#include <fstream>

Application::Application() : cfg("cfg/gbemu.conf") {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		throw gbemu::gbemu_exception{ "SDL Initialization failed!" };
}

Application::~Application() {
	SDL_Quit();
}

static void load_rom(gbemu::cpu& core, const std::string& path)
{
    gbemu::rom_file c{};
    c.load_from(path);
    core.load(c);
}

static void load_bios(gbemu::cpu& core, const std::string& path)
{
    gbemu::bios_file c{};
    c.load_from(path);
    core.load(c);
}

void Application::run() {

    gbemu::cpu core;

    auto window = SDL_CreateWindow(
        "GbEmu",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cfg.win.width, cfg.win.height,
        SDL_WINDOW_SHOWN
    );

    if (!window)
        throw gbemu::gbemu_exception{ "SDL Window creation failed!" };

    auto renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
        throw gbemu::gbemu_exception{ "SDL Renderer creation failed!" };

    SDL_RenderSetLogicalSize(renderer, 160, 144);

    // load bios(
    if (!cfg.bios.path.empty()) {
        load_bios(core, cfg.bios.path);
    }

    // load rom
    load_rom(core, cfg.rom.path);

    core.init();


    const int speed = 16;

    bool quit = false;

    // texture
    auto texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        160, 144);

    std::array<std::uint32_t, 160 * 144> framebuffer{};


    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            }
            else if (e.type == SDL_KEYDOWN) {
                // game.input(e.key.keysym.scancode);
            }
        }

        for (int i = 0; i < speed; i++) {
            core.tick();
        }
#if 0

        // (per ora) riempi con un pattern di test
        for (int y = 0; y < 144; y++)
            for (int x = 0; x < 160; x++)
                framebuffer[y * 160 + x] = ((x ^ y) & 1) ? 0xFFFFFFFF : 0xFF000000;

        SDL_UpdateTexture(texture, nullptr, framebuffer.data(), 160 * sizeof(std::uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
#endif
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}