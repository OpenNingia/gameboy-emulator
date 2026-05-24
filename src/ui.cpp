// ImGui scaffolding (docking branch) + the PR3 panel set (Display + CPU).
//
// All panel-drawing code lives here until the panel count grows enough to
// justify splitting (PR4 adds Disassembly / Memory / Breakpoints; PR5 adds
// PPU / MBC / Serial / PC-ring).  At that point this file is the seam.

#include <cstdint>

#include <SDL2/SDL.h>
#include <core.h>
#include <debugger.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_internal.h>
#include <ui.h>

namespace gbemu::ui {

    struct context {
        SDL_Window* window{nullptr};
        SDL_Renderer* renderer{nullptr};
        gbemu::debugger* dbg{nullptr};
        gbemu::core* core{nullptr};

        // Panel visibility (driven by the View menu).
        bool show_display{true};
        bool show_cpu{true};

        // Set by `View → Reset layout`; honoured at the start of the next
        // frame before DockSpace re-binds the node.
        bool layout_reset_requested{false};
    };

    namespace {

        // Default first-run dock layout: GB display on the left (taking most
        // of the area, since the user spends most of their time looking at
        // it), CPU panel on a slim right column.  PR4/PR5 panels will dock
        // into one of these two areas — extend this function as the panel
        // set grows.
        void build_default_layout(ImGuiID dockspace_id, ImVec2 size) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, size);

            ImGuiID dock_right{};
            const ImGuiID dock_left =
                ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.72f, nullptr, &dock_right);

            ImGui::DockBuilderDockWindow("Display", dock_left);
            ImGui::DockBuilderDockWindow("CPU", dock_right);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        void setup_dockspace_and_menubar(context& c) {
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->WorkPos);
            ImGui::SetNextWindowSize(vp->WorkSize);
            ImGui::SetNextWindowViewport(vp->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            constexpr ImGuiWindowFlags host_flags =
                ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            ImGui::Begin("##GbEmuDockHost", nullptr, host_flags);
            ImGui::PopStyleVar(3);

            const ImGuiID dockspace_id = ImGui::GetID("##GbEmuDockSpace");

            // First-run default OR explicit reset.  When imgui.ini restores a
            // previously customised layout, DockBuilderGetNode returns a
            // non-null node and we leave the user's choices alone.
            if (c.layout_reset_requested || ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
                c.layout_reset_requested = false;
                build_default_layout(dockspace_id, vp->Size);
            }

            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("View")) {
                    ImGui::MenuItem("Display", nullptr, &c.show_display);
                    ImGui::MenuItem("CPU", nullptr, &c.show_cpu);
                    ImGui::Separator();
                    if (ImGui::MenuItem("Reset layout"))
                        c.layout_reset_requested = true;
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::End();
        }

        void draw_display_panel(context& c, SDL_Texture* gb_texture) {
            if (!c.show_display)
                return;
            if (!ImGui::Begin("Display", &c.show_display)) {
                ImGui::End();
                return;
            }

            // Fit the 160x144 framebuffer inside the panel's available area,
            // preserving aspect ratio.  Integer scaling preferred for the
            // pixel grid, but we let it be fractional so resizing feels
            // responsive — pixel-perfect mode can land in PR5.
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            constexpr float gb_w = 160.0f;
            constexpr float gb_h = 144.0f;
            const float sx = avail.x / gb_w;
            const float sy = avail.y / gb_h;
            const float scale = (sx < sy) ? sx : sy;
            const ImVec2 img_size{gb_w * scale, gb_h * scale};

            // Centre the image in the panel.
            const ImVec2 cur = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(cur.x + (avail.x - img_size.x) * 0.5f, cur.y + (avail.y - img_size.y) * 0.5f));
            ImGui::Image(reinterpret_cast<ImTextureID>(gb_texture), img_size);

            ImGui::End();
        }

        void draw_cpu_panel(context& c) {
            if (!c.show_cpu)
                return;
            if (!ImGui::Begin("CPU", &c.show_cpu)) {
                ImGui::End();
                return;
            }

            auto& dbg = *c.dbg;
            auto& core = *c.core;
            const auto& regs = core.regs;
            const auto& cpu = core.cpu;

            // Run / Pause / Step / Step Over / Reset.
            const bool paused = dbg.is_paused();
            if (paused) {
                if (ImGui::Button("Run"))
                    dbg.resume();
            } else {
                if (ImGui::Button("Pause"))
                    dbg.pause();
            }
            ImGui::SameLine();
            const bool step_enabled = paused;
            ImGui::BeginDisabled(!step_enabled);
            if (ImGui::Button("Step"))
                (void)dbg.step();
            ImGui::SameLine();
            if (ImGui::Button("Step Over")) {
                // PR3 stub: identical to Step.  Proper step-over (run to the
                // instruction after a CALL/RST) lands once the disassembler
                // grows length-awareness in the debugger (PR4 territory).
                (void)dbg.step();
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(true);
            ImGui::Button("Reset"); // PR5: re-init core + clear RAM/VRAM
            ImGui::EndDisabled();

            ImGui::Separator();

            // 16-bit register pairs, hex.
            ImGui::Text("AF  %04X", regs.af.u16);
            ImGui::SameLine(120);
            ImGui::Text("BC  %04X", regs.bc.u16);
            ImGui::Text("DE  %04X", regs.de.u16);
            ImGui::SameLine(120);
            ImGui::Text("HL  %04X", regs.hl.u16);
            ImGui::Text("SP  %04X", regs.sp);
            ImGui::SameLine(120);
            ImGui::Text("PC  %04X", regs.pc);

            ImGui::Separator();

            // Flags decoded from F (low byte of AF).
            ImGui::Text("Z %d   N %d   H %d   C %d", regs.z_flag() ? 1 : 0, regs.n_flag() ? 1 : 0,
                        regs.h_flag() ? 1 : 0, regs.c_flag() ? 1 : 0);
            ImGui::Text("IME %d   HALT %d   STOP %d", cpu.interrupt_enabled ? 1 : 0, cpu.halted ? 1 : 0,
                        cpu.stopped ? 1 : 0);

            ImGui::Separator();

            ImGui::Text("Total cycles  %llu", static_cast<unsigned long long>(dbg.total_cycles()));
            ImGui::Text("State         %s", paused ? "Paused" : "Running");

            ImGui::End();
        }

    } // namespace

    context* init(SDL_Window* window, SDL_Renderer* renderer, debugger& dbg, gbemu::core& c) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);

        auto* ctx = new context{};
        ctx->window = window;
        ctx->renderer = renderer;
        ctx->dbg = &dbg;
        ctx->core = &c;
        return ctx;
    }

    void shutdown(context* ctx) {
        if (!ctx)
            return;
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        delete ctx;
    }

    bool process_event(context* /*ctx*/, const SDL_Event& e) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        const ImGuiIO& io = ImGui::GetIO();
        return io.WantCaptureKeyboard || io.WantCaptureMouse;
    }

    void render_frame(context* ctx, SDL_Texture* gb_texture) {
        if (!ctx)
            return;

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        setup_dockspace_and_menubar(*ctx);
        draw_display_panel(*ctx, gb_texture);
        draw_cpu_panel(*ctx);

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ctx->renderer);
    }

} // namespace gbemu::ui
