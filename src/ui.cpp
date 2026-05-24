// ImGui scaffolding (docking branch) + the PR3/PR4 panel set.
//
// Panels: Display, CPU, Disassembly, Memory (per-region tabs via the vendored
// imgui_memory_editor.h), Breakpoints + Watchpoints.  All panel-drawing code
// lives here until the panel count grows enough to justify splitting (PR5
// adds PPU / MBC / Serial / PC-ring).  At that point this file is the seam.

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <string>

#include <SDL2/SDL.h>
#include <core.h>
#include <debugger.h>
#include <disasm.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_internal.h>
#include <third_party/imgui_memory_editor.h>
#include <ui.h>

namespace gbemu::ui {

    namespace {

        // Context passed to the MemoryEditor read/write callbacks via UserData.
        struct mem_ctx {
            gbemu::core* core;
            std::uint16_t base;
        };

    } // namespace

    struct context {
        SDL_Window* window{nullptr};
        SDL_Renderer* renderer{nullptr};
        gbemu::debugger* dbg{nullptr};
        gbemu::core* core{nullptr};

        // Panel visibility (driven by the View menu).
        bool show_display{true};
        bool show_cpu{true};
        bool show_disasm{true};
        bool show_memory{true};
        bool show_breakpoints{true};

        // Set by `View → Reset layout`; honoured at the start of the next
        // frame before DockSpace re-binds the node.
        bool layout_reset_requested{false};

        // Disassembly panel persistent state.
        bool disasm_follow_pc{true};
        std::uint16_t disasm_view_addr{0x0000};
        char disasm_goto_buf[8]{};

        // Breakpoints / Watchpoints panel inputs.
        char bp_input_buf[8]{};
        char wp_input_buf[8]{};
        int wp_len_input{1};
    };

    namespace {

        // Default first-run dock layout — left column for Display (top) and
        // Memory (bottom), right column split into three rows for CPU,
        // Disassembly, and Breakpoints.  Extend here when PR5 adds PPU /
        // MBC / Serial / PC-ring panels.
        void build_default_layout(ImGuiID dockspace_id, ImVec2 size) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, size);

            ImGuiID dock_right{};
            const ImGuiID dock_left =
                ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.60f, nullptr, &dock_right);

            ImGuiID dock_left_bot{};
            const ImGuiID dock_left_top =
                ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Up, 0.55f, nullptr, &dock_left_bot);

            ImGuiID dock_right_rest{};
            const ImGuiID dock_right_top =
                ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.33f, nullptr, &dock_right_rest);

            ImGuiID dock_right_bot{};
            const ImGuiID dock_right_mid =
                ImGui::DockBuilderSplitNode(dock_right_rest, ImGuiDir_Up, 0.50f, nullptr, &dock_right_bot);

            ImGui::DockBuilderDockWindow("Display", dock_left_top);
            ImGui::DockBuilderDockWindow("Memory", dock_left_bot);
            ImGui::DockBuilderDockWindow("CPU", dock_right_top);
            ImGui::DockBuilderDockWindow("Disassembly", dock_right_mid);
            ImGui::DockBuilderDockWindow("Breakpoints", dock_right_bot);

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
                    ImGui::MenuItem("Disassembly", nullptr, &c.show_disasm);
                    ImGui::MenuItem("Memory", nullptr, &c.show_memory);
                    ImGui::MenuItem("Breakpoints", nullptr, &c.show_breakpoints);
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

            const bool paused = dbg.is_paused();
            if (paused) {
                if (ImGui::Button("Run"))
                    dbg.resume();
            } else {
                if (ImGui::Button("Pause"))
                    dbg.pause();
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(!paused);
            if (ImGui::Button("Step"))
                (void)dbg.step();
            ImGui::SameLine();
            if (ImGui::Button("Step Over")) {
                // PR3 stub: identical to Step.  Proper step-over (run to the
                // instruction after a CALL/RST) needs disassembler-length
                // awareness — landing in a follow-up.
                (void)dbg.step();
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(true);
            ImGui::Button("Reset"); // PR5: re-init core + clear RAM/VRAM
            ImGui::EndDisabled();

            ImGui::Separator();

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

            ImGui::Text("Z %d   N %d   H %d   C %d", regs.z_flag() ? 1 : 0, regs.n_flag() ? 1 : 0,
                        regs.h_flag() ? 1 : 0, regs.c_flag() ? 1 : 0);
            ImGui::Text("IME %d   HALT %d   STOP %d", cpu.interrupt_enabled ? 1 : 0, cpu.halted ? 1 : 0,
                        cpu.stopped ? 1 : 0);

            ImGui::Separator();

            ImGui::Text("Total cycles  %llu", static_cast<unsigned long long>(dbg.total_cycles()));
            ImGui::Text("State         %s", paused ? "Paused" : "Running");

            ImGui::End();
        }

        void draw_disasm_panel(context& c) {
            if (!c.show_disasm)
                return;
            if (!ImGui::Begin("Disassembly", &c.show_disasm)) {
                ImGui::End();
                return;
            }

            auto& dbg = *c.dbg;
            auto& core = *c.core;

            ImGui::Checkbox("Follow PC", &c.disasm_follow_pc);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            if (ImGui::InputText("Go to", c.disasm_goto_buf, sizeof(c.disasm_goto_buf),
                                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal)) {
                try {
                    c.disasm_view_addr = static_cast<std::uint16_t>(std::stoul(c.disasm_goto_buf, nullptr, 16));
                    c.disasm_follow_pc = false;
                } catch (...) {
                    // ignore parse failure — user can retry
                }
            }
            ImGui::Separator();

            if (c.disasm_follow_pc)
                c.disasm_view_addr = dbg.current_pc();

            ImGui::BeginChild("##DisasmList", ImVec2(0, 0), false);

            const float line_h = ImGui::GetTextLineHeightWithSpacing();
            const float avail_h = ImGui::GetContentRegionAvail().y;
            const int n = std::clamp(static_cast<int>(avail_h / line_h) + 2, 8, 64);

            std::uint16_t cur = c.disasm_view_addr;
            for (int i = 0; i < n; ++i) {
                const auto r = disasm_one(cur, core.mmu);
                const bool has_bp = dbg.breakpoint_has(cur);
                const bool is_pc = (cur == core.regs.pc);

                ImGui::PushID(static_cast<int>(cur));

                // Margin button — red square when bp set.
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
                if (has_bp)
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 60, 60, 255));
                if (ImGui::Button("##bp", ImVec2(line_h, line_h)))
                    dbg.breakpoint_toggle(cur);
                if (has_bp)
                    ImGui::PopStyleColor();
                ImGui::PopStyleVar();

                ImGui::SameLine();

                char buf[80];
                int pos = std::snprintf(buf, sizeof(buf), "%04X  ", cur);
                for (int j = 0; j < 3; ++j) {
                    if (j < r.length)
                        pos +=
                            std::snprintf(buf + pos, sizeof(buf) - static_cast<std::size_t>(pos), "%02X ", r.bytes[j]);
                    else
                        pos += std::snprintf(buf + pos, sizeof(buf) - static_cast<std::size_t>(pos), "   ");
                }
                std::snprintf(buf + pos, sizeof(buf) - static_cast<std::size_t>(pos), " %s", r.text.c_str());

                if (is_pc)
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "%s", buf);
                else
                    ImGui::TextUnformatted(buf);

                ImGui::PopID();

                if (r.length == 0)
                    break; // safety: malformed table entry
                cur = static_cast<std::uint16_t>(cur + r.length);
            }

            ImGui::EndChild();
            ImGui::End();
        }

        void draw_memory_panel(context& c) {
            if (!c.show_memory)
                return;
            if (!ImGui::Begin("Memory", &c.show_memory)) {
                ImGui::End();
                return;
            }

            struct region {
                const char* name;
                std::uint16_t base;
                std::uint32_t size;
            };
            static constexpr std::array<region, 7> regions = {{
                {"ROM", 0x0000, 0x8000},
                {"VRAM", 0x8000, 0x2000},
                {"ERAM", 0xA000, 0x2000},
                {"WRAM", 0xC000, 0x2000},
                {"OAM", 0xFE00, 0x00A0},
                {"I/O", 0xFF00, 0x0080},
                {"HRAM", 0xFF80, 0x0080},
            }};

            // One MemoryEditor per region so cursor/selection state doesn't
            // leak across tab switches.  All editors share callbacks that
            // route through mmu::read_u8/write_u8 — the per-tab base address
            // is carried in `mem_ctx` via UserData.
            static std::array<MemoryEditor, regions.size()> editors;
            static std::array<mem_ctx, regions.size()> ctxs;
            static bool editors_init = false;
            if (!editors_init) {
                editors_init = true;
                for (auto& ed : editors) {
                    ed.ReadFn = [](const ImU8*, std::size_t off, void* ud) -> ImU8 {
                        auto* mc = static_cast<mem_ctx*>(ud);
                        return mc->core->mmu.read_u8(static_cast<std::uint16_t>(mc->base + off));
                    };
                    ed.WriteFn = [](ImU8*, std::size_t off, ImU8 d, void* ud) {
                        auto* mc = static_cast<mem_ctx*>(ud);
                        mc->core->mmu.write_u8(static_cast<std::uint16_t>(mc->base + off), d);
                    };
                    ed.OptShowOptions = false;
                }
            }

            if (ImGui::BeginTabBar("##MemTabs")) {
                for (std::size_t i = 0; i < regions.size(); ++i) {
                    if (ImGui::BeginTabItem(regions[i].name)) {
                        ctxs[i] = mem_ctx{c.core, regions[i].base};
                        editors[i].UserData = &ctxs[i];
                        editors[i].DrawContents(nullptr, regions[i].size, regions[i].base);
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }

            ImGui::End();
        }

        void draw_breakpoints_panel(context& c) {
            if (!c.show_breakpoints)
                return;
            if (!ImGui::Begin("Breakpoints", &c.show_breakpoints)) {
                ImGui::End();
                return;
            }

            auto& dbg = *c.dbg;

            // --- Breakpoints ---
            ImGui::SeparatorText("Breakpoints");
            ImGui::SetNextItemWidth(80);
            ImGui::InputText("Addr##bp", c.bp_input_buf, sizeof(c.bp_input_buf), ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::SameLine();
            if (ImGui::Button("Add##bp")) {
                try {
                    if (c.bp_input_buf[0]) {
                        const auto addr = static_cast<std::uint16_t>(std::stoul(c.bp_input_buf, nullptr, 16));
                        dbg.breakpoint_set(addr);
                        c.bp_input_buf[0] = '\0';
                    }
                } catch (...) {}
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear all##bp")) {
                for (auto a : dbg.breakpoint_list())
                    dbg.breakpoint_clear(a);
            }

            if (ImGui::BeginListBox("##bp_list", ImVec2(-FLT_MIN, 6 * ImGui::GetTextLineHeightWithSpacing()))) {
                for (auto a : dbg.breakpoint_list()) {
                    ImGui::PushID(static_cast<int>(a));
                    if (ImGui::SmallButton("X"))
                        dbg.breakpoint_clear(a);
                    ImGui::SameLine();
                    ImGui::Text("$%04X", a);
                    ImGui::PopID();
                }
                ImGui::EndListBox();
            }

            // --- Watchpoints ---
            ImGui::SeparatorText("Watchpoints");
            ImGui::SetNextItemWidth(80);
            ImGui::InputText("Addr##wp", c.wp_input_buf, sizeof(c.wp_input_buf), ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            ImGui::InputInt("Len##wp", &c.wp_len_input, 1, 1);
            if (c.wp_len_input < 1)
                c.wp_len_input = 1;
            ImGui::SameLine();
            if (ImGui::Button("Add##wp")) {
                try {
                    if (c.wp_input_buf[0]) {
                        const auto addr = static_cast<std::uint16_t>(std::stoul(c.wp_input_buf, nullptr, 16));
                        dbg.watchpoint_set(addr, static_cast<std::uint16_t>(c.wp_len_input));
                        c.wp_input_buf[0] = '\0';
                    }
                } catch (...) {}
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear all##wp"))
                dbg.watchpoint_clear_all();

            if (ImGui::BeginListBox("##wp_list", ImVec2(-FLT_MIN, 6 * ImGui::GetTextLineHeightWithSpacing()))) {
                // Snapshot because the underlying InlinedVector may shrink
                // if Remove is clicked during this iteration.
                auto wps = dbg.watchpoints();
                for (const auto& w : wps) {
                    ImGui::PushID(static_cast<int>(w.addr));
                    if (ImGui::SmallButton("X"))
                        dbg.watchpoint_clear(w.addr);
                    ImGui::SameLine();

                    // Live value display: read up to 4 bytes via mmu and show
                    // hex; truncate longer ranges with "..." since rendering
                    // the full payload would dwarf the rest of the row.
                    char val_buf[64];
                    int pos = 0;
                    const std::uint16_t display_len = std::min<std::uint16_t>(w.len, 4);
                    for (std::uint16_t i = 0; i < display_len; ++i) {
                        pos += std::snprintf(val_buf + pos, sizeof(val_buf) - static_cast<std::size_t>(pos), "%s%02X",
                                             i == 0 ? "" : " ",
                                             c.core->mmu.read_u8(static_cast<std::uint16_t>(w.addr + i)));
                    }
                    if (w.len > display_len)
                        std::snprintf(val_buf + pos, sizeof(val_buf) - static_cast<std::size_t>(pos), " ...");

                    ImGui::Text("$%04X len=%u = %s", w.addr, static_cast<unsigned>(w.len), val_buf);
                    ImGui::PopID();
                }
                ImGui::EndListBox();
            }

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
        draw_disasm_panel(*ctx);
        draw_memory_panel(*ctx);
        draw_breakpoints_panel(*ctx);

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ctx->renderer);
    }

} // namespace gbemu::ui
