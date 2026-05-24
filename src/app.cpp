#include <cstdio>
#include <fstream>
#include <iostream>

#include <SDL2/SDL.h>
#include <app.h>
#include <apu.h>
#include <audio_ring_buffer.hpp>
#include <core.h>
#include <debugger.h>
#include <exc.hpp>
#include <joypad.h>
#include <ui.h>

namespace gbemu {
    // Defined in src/script_runner.cpp.
    int run_script(debugger& dbg, const std::string& script_path, const std::string& out_path);
} // namespace gbemu

namespace {
    // Initial SDL window dimensions used the first time the app runs (no
    // gbemu_window.state on disk yet).  Subsequent launches restore the
    // user's last size from gbemu_window.state.
    constexpr int DEFAULT_WINDOW_WIDTH = 1280;
    constexpr int DEFAULT_WINDOW_HEIGHT = 720;

    // Small persistent window-state file kept next to imgui.ini in CWD.  SDL
    // owns the platform window outside ImGui, so the window size is not
    // covered by imgui.ini; without this file the window would snap back to
    // the hard-coded default at every launch.  Format is a single line
    // "WIDTH HEIGHT".
    constexpr const char* WINDOW_STATE_FILE = "gbemu_window.state";

    bool load_window_state(int& w, int& h) {
        std::ifstream f(WINDOW_STATE_FILE);
        int rw = 0, rh = 0;
        if (f >> rw >> rh && rw > 0 && rh > 0) {
            w = rw;
            h = rh;
            return true;
        }
        return false;
    }

    void save_window_state(int w, int h) {
        std::ofstream f(WINDOW_STATE_FILE);
        if (f)
            f << w << ' ' << h << '\n';
    }

    // SDL audio callback — invoked on the SDL audio thread when the device
    // wants more samples.  Drains stereo float frames out of the APU's ring
    // buffer; on underrun (ring empty) the remaining slots are filled with
    // silence so the audio device keeps consuming on schedule.
    void SDLCALL audio_callback(void* userdata, Uint8* stream, int len) {
        auto* ring = static_cast<gbemu::audio_ring_buffer*>(userdata);
        auto* out = reinterpret_cast<float*>(stream);
        const int frames = len / int(2 * sizeof(float));
        for (int i = 0; i < frames; ++i) {
            float l = 0.0f, r = 0.0f;
            ring->pop(l, r); // pop() leaves l/r at 0 on underrun
            out[i * 2] = l;
            out[i * 2 + 1] = r;
        }
    }
} // namespace

namespace {
    // Hard-coded keyboard bindings. Arrow keys drive the D-pad; Z/X are A/B
    // (typical fceux/SameBoy convention so the player's right hand sits
    // naturally over them); Backspace = Select, Enter = Start. Returns
    // false for unmapped keys so the caller can early-out.
    bool map_keycode_to_button(SDL_Keycode k, gbemu::joypad::button& out) {
        using b = gbemu::joypad::button;
        switch (k) {
            case SDLK_UP:
                out = b::up;
                return true;
            case SDLK_DOWN:
                out = b::down;
                return true;
            case SDLK_LEFT:
                out = b::left;
                return true;
            case SDLK_RIGHT:
                out = b::right;
                return true;
            case SDLK_z:
                out = b::a;
                return true;
            case SDLK_x:
                out = b::b;
                return true;
            case SDLK_BACKSPACE:
                out = b::select;
                return true;
            case SDLK_RETURN:
                out = b::start;
                return true;
            default:
                return false;
        }
    }

    // Gamepad bindings. SDL_GameController normalizes vendor layouts (Xbox,
    // PlayStation, Switch Pro, generic) onto a single virtual layout, so we
    // can hard-code SDL's symbolic buttons here. GB-A maps to gamepad-A
    // (south on Xbox-style pads), GB-B maps to gamepad-X (west) — the
    // industry-standard mapping that keeps the "B" button as the secondary
    // action under the thumb's resting position.
    bool map_controller_button_to_button(Uint8 sdl_btn, gbemu::joypad::button& out) {
        using b = gbemu::joypad::button;
        switch (sdl_btn) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                out = b::up;
                return true;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                out = b::down;
                return true;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                out = b::left;
                return true;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                out = b::right;
                return true;
            case SDL_CONTROLLER_BUTTON_A:
                out = b::a;
                return true;
            case SDL_CONTROLLER_BUTTON_X:
                out = b::b;
                return true;
            case SDL_CONTROLLER_BUTTON_BACK:
                out = b::select;
                return true;
            case SDL_CONTROLLER_BUTTON_START:
                out = b::start;
                return true;
            default:
                return false;
        }
    }
} // namespace

Application::Application() : cfg("cfg/gbemu.conf") {
    // SDL_INIT_GAMECONTROLLER pulls in SDL_INIT_JOYSTICK and SDL_INIT_EVENTS;
    // we rely on it for both the SDL_GameController API used below and the
    // automatic SDL_CONTROLLERDEVICEADDED events fired at startup for
    // already-connected pads.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
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

    // The debugger's own $FF02 handler (installed in its ctor) is now the
    // single canonical serial sink — read by the headless `serial-dump`
    // command and rendered live by the ImGui Serial panel.  The previous
    // stdout-echo handler was retired in PR5.

    // load bios
    if (!cfg.bios.path.empty() && !cfg.bios.skip) {
        load_bios(core, cfg.bios.path);
    }

    // load rom
    load_rom(core, cfg.rom.path);

    core.init();

    if (headless_) {
        // No window, no renderer, no event loop — just run the script and
        // return.  PPU keeps running internally; its framebuffer is just
        // never presented, so VBlank edges remain observable by run-until.
        // APU output stays disabled in headless mode: nobody is draining the
        // ring buffer, and enabling push would deadlock at the first full
        // buffer.  Sample-pacing counters still advance, so cycle accounting
        // is unaffected.
        gbemu::run_script(debugger, script_path_, output_path_);
        return;
    }

    // Open the audio device once the core (and therefore the APU's ring
    // buffer) exists.  Format is 48 kHz / F32 / stereo to match
    // gbemu::apu::SAMPLE_RATE.  `samples=1024` is the per-callback frame count
    // — small enough to keep latency low (~21 ms at 48 kHz) without thrashing
    // the audio thread.  We use SDL_AUDIO_ALLOW_FREQUENCY_CHANGE=0 (default)
    // so SDL is forced to give us exactly the rate we asked for; otherwise
    // the APU's sample cadence would drift from the actual device rate.
    SDL_AudioDeviceID audio_dev = 0;
    {
        SDL_AudioSpec want{}, have{};
        want.freq = gbemu::apu::SAMPLE_RATE;
        want.format = AUDIO_F32SYS;
        want.channels = 2;
        want.samples = 1024;
        want.callback = audio_callback;
        want.userdata = &core.apu.output();
        audio_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (audio_dev == 0)
            throw gbemu::gbemu_exception{"SDL_OpenAudioDevice failed!"};
        core.apu.enable_output(true);
        SDL_PauseAudioDevice(audio_dev, 0); // start the audio thread
    }

    // Restore last-used SDL window dimensions if available; otherwise fall
    // back to the hard-coded default (first launch on this machine).
    int win_w = DEFAULT_WINDOW_WIDTH;
    int win_h = DEFAULT_WINDOW_HEIGHT;
    load_window_state(win_w, win_h);

    auto window = SDL_CreateWindow("GbEmu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window)
        throw gbemu::gbemu_exception{"SDL Window creation failed!"};

    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
        throw gbemu::gbemu_exception{"SDL Renderer creation failed!"};

    auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

    auto* ui_ctx = gbemu::ui::init(window, renderer, debugger, core);

    // Open the first available game controller, if any. The matching
    // SDL_CONTROLLERDEVICEADDED event also fires in the main loop, so
    // hotplug works the same way — this just covers the case where a
    // controller was already plugged in at startup.
    SDL_GameController* controller = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller)
                break;
        }
    }

    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            const bool imgui_captured = gbemu::ui::process_event(ui_ctx, e);

            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            } else if (e.type == SDL_KEYDOWN && !imgui_captured) {
                // F12: diagnostic dump of registers + recent PCs.  Routed
                // through the debugger so the format matches the headless
                // script runner.
                if (e.key.keysym.sym == SDLK_F12) {
                    std::cout << "=== regs @cycle=" << debugger.total_cycles() << " ===\n";
                    debugger.dump_regs(std::cout);
                    std::cout << "=== pc-ring 32 ===\n";
                    debugger.dump_pc_ring(std::cout, 32);
                    std::cout.flush();
                }
                gbemu::joypad::button btn;
                if (!e.key.repeat && map_keycode_to_button(e.key.keysym.sym, btn))
                    core.joypad.set_button(btn, true);
            } else if (e.type == SDL_KEYUP && !imgui_captured) {
                gbemu::joypad::button btn;
                if (map_keycode_to_button(e.key.keysym.sym, btn))
                    core.joypad.set_button(btn, false);
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                gbemu::joypad::button btn;
                if (map_controller_button_to_button(e.cbutton.button, btn))
                    core.joypad.set_button(btn, true);
            } else if (e.type == SDL_CONTROLLERBUTTONUP) {
                gbemu::joypad::button btn;
                if (map_controller_button_to_button(e.cbutton.button, btn))
                    core.joypad.set_button(btn, false);
            } else if (e.type == SDL_CONTROLLERDEVICEADDED) {
                // Hot-plug: claim the new pad only if we don't already
                // have one open. e.cdevice.which is a *joystick index* on
                // CONTROLLERDEVICEADDED (per SDL docs), suitable for
                // SDL_GameControllerOpen.
                if (!controller)
                    controller = SDL_GameControllerOpen(e.cdevice.which);
            } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
                // Here e.cdevice.which is an *instance id*. Match against
                // the open controller's instance id and close if it's the
                // one that went away.
                if (controller) {
                    SDL_Joystick* j = SDL_GameControllerGetJoystick(controller);
                    if (j && SDL_JoystickInstanceID(j) == e.cdevice.which) {
                        SDL_GameControllerClose(controller);
                        controller = nullptr;
                    }
                }
            }
        }

        // Step the emulator only when the CPU panel hasn't paused it.  ImGui
        // keeps drawing either way so the user can inspect state and use the
        // Step / Step Over buttons.  In Run mode we route through
        // `run_until(none)` so that breakpoints and watchpoints set via the UI
        // actually fire — they auto-pause the debugger when they hit.
        if (!debugger.is_paused()) {
            // 4.19 MHz / 60 fps ≈ 69905 T-cycles per frame
            constexpr std::uint64_t CYCLES_PER_FRAME = 70224; // valore esatto DMG
            gbemu::stop_condition cond{};
            cond.kind = gbemu::stop_kind::none;
            const auto rr = debugger.run_until(cond, CYCLES_PER_FRAME);
            if (rr.outcome == gbemu::run_outcome::breakpoint || rr.outcome == gbemu::run_outcome::watchpoint) {
                debugger.pause();
            }
        }

        // Refresh the GB framebuffer texture on every new frame ready edge.
        // When paused the texture keeps showing the last produced frame.
        if (core.ppu.consume_frame_ready()) {
            SDL_UpdateTexture(texture, nullptr, core.ppu.framebuffer(), 160 * 4);
        }

        SDL_RenderClear(renderer);
        gbemu::ui::render_frame(ui_ctx, texture);
        SDL_RenderPresent(renderer);
    }

    // Snapshot final window dimensions before teardown so the next launch
    // re-opens at the same size.
    {
        int final_w = 0, final_h = 0;
        SDL_GetWindowSize(window, &final_w, &final_h);
        save_window_state(final_w, final_h);
    }

    // Close the audio device before the core (and the ring buffer it owns)
    // goes out of scope, otherwise the audio thread could fire a final
    // callback into freed memory.  enable_output(false) belt-and-braces
    // prevents any in-flight emit_sample from blocking on a ring nobody is
    // draining anymore.
    if (audio_dev) {
        SDL_PauseAudioDevice(audio_dev, 1);
        core.apu.enable_output(false);
        SDL_CloseAudioDevice(audio_dev);
    }

    if (controller)
        SDL_GameControllerClose(controller);

    gbemu::ui::shutdown(ui_ctx);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
