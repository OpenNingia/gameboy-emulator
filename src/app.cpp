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
    gbemu::cartridge c{};
    c.load_rom(path);
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

    // load rom
    load_rom(core, cfg.rom.rom_path);


    const int speed = 16;

    bool quit = false;
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
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}