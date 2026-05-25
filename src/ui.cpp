// Panels: Display, CPU, Disassembly, Memory (per-region tabs via the vendored
// imgui_memory_editor.h), Breakpoints + Watchpoints, PPU (LCDC/STAT decode +
// palette swatches + VRAM tile viewer + BG tile-map viewer), MBC (banking
// state via mbc::debug_state), Serial (scrollback of debugger serial buffer),
// PC-ring (clickable history of recent PCs).  Kept in one file — the panel
// count is high but each panel is small and the shared `context` lookup is
// trivial.  If this grows past ~1.5kLOC consider splitting per-panel.

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <cfg.h>
#include <core.h>
#include <debugger.h>
#include <disasm.h>
#include <display_post.h>
#include <gb_layout.h>
#include <gfx.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_internal.h>
#include <log.h>
#include <mbc.h>
#include <pixel_pipeline.h>
#include <third_party/imgui_memory_editor.h>
#include <ui.h>

namespace gbemu::ui {

    namespace {

        // Context passed to the MemoryEditor read/write callbacks via UserData.
        struct mem_ctx {
            gbemu::core* core;
            std::uint16_t base;
        };

        // Persistent state files kept next to imgui.ini in CWD.
        constexpr const char* RECENT_ROMS_FILE = "gbemu_recent.txt";
        constexpr std::size_t MAX_RECENT_ROMS = 8;

    } // namespace

    struct context {
        SDL_Window* window{nullptr};
        SDL_Renderer* renderer{nullptr};
        gbemu::gfx::backend* backend{nullptr};
        gbemu::debugger* dbg{nullptr};
        gbemu::core* core{nullptr};

        // Streaming presenter for the GB framebuffer shown in the Display
        // panel.  Created eagerly in ui::init; uploaded each time the PPU
        // signals a new frame.
        gbemu::gfx::presenter* display_present{nullptr};

        // Panel visibility (driven by the View menu).
        bool show_display{true};
        bool show_cpu{true};
        bool show_disasm{true};
        bool show_memory{true};
        bool show_breakpoints{true};
        bool show_ppu{true};
        bool show_mbc{true};
        bool show_serial{true};
        bool show_pc_ring{true};

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

        // PPU panel: streaming presenters for the VRAM tile grid (128x192
        // px, 16x24 tiles of 8x8) and the BG tile map viewer (256x256 px).
        // Both are created lazily on first use and recycled across frames.
        // The map_idx flag selects which of the two BG maps to view ($9800
        // vs $9C00); independent of LCDC.3 so the user can inspect either.
        gbemu::gfx::presenter* ppu_tiles_present{nullptr};
        gbemu::gfx::presenter* ppu_bgmap_present{nullptr};
        int ppu_bgmap_idx{0}; // 0 → $9800, 1 → $9C00

        // Menu-bar / popup state.
        host_actions actions{};
        std::vector<std::string> recent_roms{};
        bool show_about_popup{false};

        // Display post-processing — frame blending (LCD ghosting) and the
        // palette registry feeding the PPU's resolver.  Owned by the UI
        // context because both are interactive-only: the headless script
        // runner never calls ui::render_frame and therefore never touches
        // this state.  The Application configures both from cfg.display
        // during ui::init.
        gbemu::display::frame_blender post{};
        gbemu::display::palette_registry palettes{};
        // Track the currently-active palette name so the menu can render
        // radio checkmarks without re-walking the registry to identify it.
        std::string active_palette_name{"grey"};
    };

    namespace {

        // Recents persistence: one path per line, MRU first. Missing file is
        // not an error (first launch). Save is best-effort.
        void load_recent_roms(context& c) {
            c.recent_roms.clear();
            std::ifstream f(RECENT_ROMS_FILE);
            std::string line;
            while (std::getline(f, line) && c.recent_roms.size() < MAX_RECENT_ROMS) {
                if (!line.empty())
                    c.recent_roms.push_back(line);
            }
        }

        void save_recent_roms(const context& c) {
            std::ofstream f(RECENT_ROMS_FILE);
            if (!f)
                return;
            for (const auto& p : c.recent_roms)
                f << p << '\n';
        }

    } // namespace

    namespace {

        // Default first-run dock layout.  Three columns:
        //   - Left (40%): Display (top), Memory + PPU as tabs (bottom).
        //   - Middle (35%): CPU (top), Disassembly (bottom).
        //   - Right (25%): Breakpoints + MBC tabs (top), Serial + PC-ring
        //     tabs (bottom).
        // Tabbed panels share a dock node — users can tear them off via the
        // tab handles.  The check in setup_dockspace_and_menubar against
        // DockBuilderGetNode preserves any post-first-launch customisation.
        void build_default_layout(ImGuiID dockspace_id, ImVec2 size) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, size);

            ImGuiID dock_rest{};
            const ImGuiID dock_left =
                ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.40f, nullptr, &dock_rest);

            ImGuiID dock_right{};
            const ImGuiID dock_middle =
                ImGui::DockBuilderSplitNode(dock_rest, ImGuiDir_Left, 0.58f, nullptr, &dock_right);

            ImGuiID dock_left_bot{};
            const ImGuiID dock_left_top =
                ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Up, 0.55f, nullptr, &dock_left_bot);

            ImGuiID dock_middle_bot{};
            const ImGuiID dock_middle_top =
                ImGui::DockBuilderSplitNode(dock_middle, ImGuiDir_Up, 0.40f, nullptr, &dock_middle_bot);

            ImGuiID dock_right_bot{};
            const ImGuiID dock_right_top =
                ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.50f, nullptr, &dock_right_bot);

            ImGui::DockBuilderDockWindow("Display", dock_left_top);
            ImGui::DockBuilderDockWindow("Memory", dock_left_bot);
            ImGui::DockBuilderDockWindow("PPU", dock_left_bot);
            ImGui::DockBuilderDockWindow("CPU", dock_middle_top);
            ImGui::DockBuilderDockWindow("Disassembly", dock_middle_bot);
            ImGui::DockBuilderDockWindow("Breakpoints", dock_right_top);
            ImGui::DockBuilderDockWindow("MBC", dock_right_top);
            ImGui::DockBuilderDockWindow("Serial", dock_right_bot);
            ImGui::DockBuilderDockWindow("PC ring", dock_right_bot);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        // True when a cartridge is attached.  Used to gate emulation controls
        // (Resume/Pause/Reset, Step/Step Over) — without a cart the MMU
        // returns open-bus for every cart read, so "running" just executes
        // 0xFF bytes (or, with BIOS, paints garbage in place of the logo).
        bool has_rom(const context& c) {
            return c.core && c.core->mmu.cart() != nullptr;
        }

        void draw_menu_bar(context& c) {
            auto& dbg = *c.dbg;

            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Load ROM...", "Ctrl+O"))
                    c.actions.load_rom_dialog_requested = true;

                // Recent ROMs submenu — disabled (and shows "(none)") when
                // the MRU list is empty so the user gets the feedback that
                // it exists but has nothing to offer yet.
                if (ImGui::BeginMenu("Recent ROMs", !c.recent_roms.empty())) {
                    for (std::size_t i = 0; i < c.recent_roms.size(); ++i) {
                        // PushID guards against duplicate basenames showing
                        // up in the list (rare but possible across folders).
                        ImGui::PushID(static_cast<int>(i));
                        if (ImGui::MenuItem(c.recent_roms[i].c_str()))
                            c.actions.pending_rom_load = c.recent_roms[i];
                        ImGui::PopID();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Clear list")) {
                        c.recent_roms.clear();
                        save_recent_roms(c);
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Alt+F4"))
                    c.actions.quit_requested = true;

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Emulation")) {
                const bool paused = dbg.is_paused();
                const bool rom_loaded = has_rom(c);
                ImGui::BeginDisabled(!rom_loaded);
                if (paused) {
                    if (ImGui::MenuItem("Resume", "Space"))
                        dbg.resume();
                } else {
                    if (ImGui::MenuItem("Pause", "Space"))
                        dbg.pause();
                }
                if (ImGui::MenuItem("Reset", "Ctrl+R")) {
                    dbg.reset();
                    // Clear the blender history so the first post-reset frame
                    // is shown un-ghosted (matches the user's mental model of
                    // "power-cycle").
                    c.post.reset();
                }
                ImGui::EndDisabled();

                // Speed and Save/Load State live behind TODO §3 and §2
                // respectively.  Surfaced as disabled submenus so users see
                // the placeholder rather than wondering where these features
                // will land.
                if (ImGui::BeginMenu("Speed", false)) {
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Save State", false)) {
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Load State", false)) {
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                const bool is_fs = c.window && (SDL_GetWindowFlags(c.window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                if (ImGui::MenuItem("Toggle Fullscreen", "F11", is_fs)) {
                    SDL_SetWindowFullscreen(c.window, is_fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                }

                // Display ▸ — frame blending (LCD ghosting) + palette swap.
                // Live state lives in the context; the cfg.display values
                // are only the initial defaults at boot.
                if (ImGui::BeginMenu("Display")) {
                    using gbemu::display::blend_mode;
                    if (ImGui::BeginMenu("Frame Blending")) {
                        const blend_mode current = c.post.mode();
                        if (ImGui::MenuItem("Disabled", nullptr, current == blend_mode::disabled))
                            c.post.set_mode(blend_mode::disabled);
                        if (ImGui::MenuItem("Simple", nullptr, current == blend_mode::simple))
                            c.post.set_mode(blend_mode::simple);
                        if (ImGui::MenuItem("Accurate", nullptr, current == blend_mode::accurate))
                            c.post.set_mode(blend_mode::accurate);
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Palette")) {
                        // Built-ins are inserted first by install_builtins()
                        // and any user .sbp files follow.  A separator marks
                        // the boundary so users can tell at a glance what is
                        // custom vs. ours.  The first 4 entries are the
                        // built-ins (grey/dmg/mgb/gbl), see palette_registry.
                        const auto& list = c.palettes.all();
                        for (std::size_t i = 0; i < list.size(); ++i) {
                            if (i == 4 && list.size() > 4)
                                ImGui::Separator();
                            const bool selected = (list[i].name == c.active_palette_name);
                            if (ImGui::MenuItem(list[i].name.c_str(), nullptr, selected)) {
                                c.core->ppu.set_palette(list[i].shades);
                                c.active_palette_name = list[i].name;
                            }
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Display", nullptr, &c.show_display);
                ImGui::MenuItem("CPU", nullptr, &c.show_cpu);
                ImGui::MenuItem("Disassembly", nullptr, &c.show_disasm);
                ImGui::MenuItem("Memory", nullptr, &c.show_memory);
                ImGui::MenuItem("Breakpoints", nullptr, &c.show_breakpoints);
                ImGui::MenuItem("PPU", nullptr, &c.show_ppu);
                ImGui::MenuItem("MBC", nullptr, &c.show_mbc);
                ImGui::MenuItem("Serial", nullptr, &c.show_serial);
                ImGui::MenuItem("PC ring", nullptr, &c.show_pc_ring);
                ImGui::Separator();
                if (ImGui::MenuItem("Reset layout"))
                    c.layout_reset_requested = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("About")) {
                if (ImGui::MenuItem("About GbEmu..."))
                    c.show_about_popup = true;
                ImGui::EndMenu();
            }
        }

        void draw_about_popup(context& c) {
            // Opening must happen *inside* the BeginPopupModal's parent
            // window scope, hence the two-step "request flag → OpenPopup
            // here" dance.
            if (c.show_about_popup) {
                c.show_about_popup = false;
                ImGui::OpenPopup("About GbEmu");
            }

            // Centre the popup on the viewport — auto-resize keeps it tight
            // around the content regardless of font scaling.
            const ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("About GbEmu", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextUnformatted("GbEmu");
                ImGui::TextDisabled("Game Boy emulator");
                ImGui::Separator();
                ImGui::Text("Version    0.1.0-dev");
                ImGui::Text("Built      %s %s", __DATE__, __TIME__);
                SDL_version sdlv;
                SDL_GetVersion(&sdlv);
                ImGui::Text("SDL2       %u.%u.%u", sdlv.major, sdlv.minor, sdlv.patch);
                ImGui::Text("ImGui      %s", ImGui::GetVersion());
                ImGui::Separator();

                if (ImGui::Button("Copy version info")) {
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "GbEmu 0.1.0-dev | SDL2 %u.%u.%u | ImGui %s | built %s %s",
                                  sdlv.major, sdlv.minor, sdlv.patch, ImGui::GetVersion(), __DATE__, __TIME__);
                    ImGui::SetClipboardText(buf);
                }
                ImGui::SameLine();
                if (ImGui::Button("OK"))
                    ImGui::CloseCurrentPopup();

                ImGui::EndPopup();
            }
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
                draw_menu_bar(c);
                ImGui::EndMenuBar();
            }

            // About popup must be drawn inside the host window scope so it
            // inherits the viewport — keep it adjacent to the menu bar.
            draw_about_popup(c);

            ImGui::End();
        }

        void draw_display_panel(context& c) {
            if (!c.show_display)
                return;
            if (!ImGui::Begin("Display", &c.show_display)) {
                ImGui::End();
                return;
            }

            // Fit the GB framebuffer inside the panel's available area,
            // preserving aspect ratio.  Integer scaling preferred for the
            // pixel grid, but we let it be fractional so resizing feels
            // responsive — pixel-perfect mode can land in PR5.
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            constexpr float gb_w = static_cast<float>(gb::LCD_WIDTH);
            constexpr float gb_h = static_cast<float>(gb::LCD_HEIGHT);
            const float sx = avail.x / gb_w;
            const float sy = avail.y / gb_h;
            const float scale = (sx < sy) ? sx : sy;
            const ImVec2 img_size{gb_w * scale, gb_h * scale};

            const ImVec2 cur = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(cur.x + (avail.x - img_size.x) * 0.5f, cur.y + (avail.y - img_size.y) * 0.5f));
            if (c.display_present)
                ImGui::Image(gbemu::gfx::presenter_imgui_id(c.display_present), img_size);

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
            const bool rom_loaded = has_rom(c);
            ImGui::BeginDisabled(!rom_loaded);
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
            if (ImGui::Button("Step Over"))
                (void)dbg.step_over();
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Reset"))
                dbg.reset();
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
                {"ROM", gb::ROM_BASE, gb::ROM_VISIBLE_SIZE},
                {"VRAM", gb::VRAM_BASE, gb::VRAM_SIZE},
                {"ERAM", gb::ERAM_BASE, gb::ERAM_SIZE},
                {"WRAM", gb::WRAM_BASE, gb::WRAM_SIZE},
                {"OAM", gb::OAM_BASE, gb::OAM_TOTAL_BYTES},
                {"I/O", gb::io::BASE, gb::IO_REGION_SIZE},
                {"HRAM", gb::HRAM_BASE, gb::HRAM_SIZE},
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

        // ---------- PR5: PPU / MBC / Serial / PC-ring ----------

        // Decode an 8x8 GB tile (TILE_BYTES bytes at vram_offset) into `out`
        // (a row-major buffer of dst_stride pixels per row), placing the
        // top-left corner of the tile at (dst_x, dst_y).  Each output pixel is
        // resolved through `resolver` against `pal_id` (the viewer hardcodes
        // palette_id::bg, matching the BG/window path in the live PPU).
        void decode_tile_8x8(const std::uint8_t* vram, std::uint32_t vram_offset, std::uint32_t* out,
                             std::uint32_t dst_stride, std::uint32_t dst_x, std::uint32_t dst_y,
                             const gbemu::palette_resolver& resolver, gbemu::palette_id pal_id) {
            for (std::uint32_t row = 0; row < gb::TILE_PIXELS; ++row) {
                const std::uint8_t lo = vram[vram_offset + row * 2];
                const std::uint8_t hi = vram[vram_offset + row * 2 + 1];
                for (std::uint32_t col = 0; col < gb::TILE_PIXELS; ++col) {
                    const std::uint8_t shift = static_cast<std::uint8_t>(7 - col);
                    const std::uint8_t ci = gbemu::tile_color_index(lo, hi, shift);
                    out[(dst_y + row) * dst_stride + (dst_x + col)] = resolver.resolve(pal_id, ci);
                }
            }
        }

        // Repaint the 16x24 tile-data grid covering VRAM $8000-$97FF.  Tiles
        // are laid out row-major: index 0 top-left, index 15 top-right,
        // index 16 second row, etc.  Result is uploaded into the tile
        // presenter (lazily allocated on first call).
        void refresh_tile_viewer_texture(context& c) {
            if (!c.ppu_tiles_present) {
                c.ppu_tiles_present = gbemu::gfx::presenter_create(c.backend, 128, 192);
                if (!c.ppu_tiles_present)
                    return;
            }
            std::array<std::uint32_t, 128 * 192> pixels{};
            const auto& resolver = c.core->ppu.palette();
            const std::uint8_t* vram = c.core->mmu.vram_bank().data();
            // The viewer covers $8000-$97FF: 384 tiles laid out 16 wide × 24 tall.
            constexpr std::uint32_t tiles_per_row = 16;
            constexpr std::uint32_t total_tiles = 384;
            for (std::uint32_t t = 0; t < total_tiles; ++t) {
                const std::uint32_t tx = (t % tiles_per_row) * gb::TILE_PIXELS;
                const std::uint32_t ty = (t / tiles_per_row) * gb::TILE_PIXELS;
                decode_tile_8x8(vram, t * gb::TILE_BYTES, pixels.data(), 128, tx, ty, resolver, gbemu::palette_id::bg);
            }
            gbemu::gfx::presenter_upload(c.ppu_tiles_present, pixels.data());
        }

        // Repaint a 256x256 BG tile-map viewer.  Reads tile indices from the
        // map area selected by c.ppu_bgmap_idx ($9800 vs $9C00) and resolves
        // tile data through the current LCDC.4 addressing mode (the same
        // logic ppu::render_bg_scanline uses).
        void refresh_bgmap_viewer_texture(context& c) {
            if (!c.ppu_bgmap_present) {
                c.ppu_bgmap_present = gbemu::gfx::presenter_create(c.backend, 256, 256);
                if (!c.ppu_bgmap_present)
                    return;
            }
            std::array<std::uint32_t, 256 * 256> pixels{};
            const std::uint8_t lcdc = c.core->mmu.hwr_lcdc();
            const auto& resolver = c.core->ppu.palette();
            const bool data_8000 = (lcdc & gb::lcdc::tile_data_8000) != 0;
            const std::uint16_t map_base = c.ppu_bgmap_idx ? gb::BG_MAP_1 : gb::BG_MAP_0;
            const std::uint8_t* vram = c.core->mmu.vram_bank().data();
            // Signed-mode tile data window is centred at $9000 — offset
            // gb::TILE_DATA_SIGNED_BASE - gb::VRAM_BASE = $1000 inside the VRAM array.
            constexpr std::uint32_t signed_window_off = gb::TILE_DATA_SIGNED_BASE - gb::VRAM_BASE;
            for (std::uint32_t ty = 0; ty < gb::TILES_PER_MAP_ROW; ++ty) {
                for (std::uint32_t tx = 0; tx < gb::TILES_PER_MAP_ROW; ++tx) {
                    const std::uint8_t idx = vram[(map_base - gb::VRAM_BASE) + ty * gb::TILES_PER_MAP_ROW + tx];
                    const std::uint32_t tile_off =
                        data_8000 ? static_cast<std::uint32_t>(idx) * gb::TILE_BYTES
                                  : static_cast<std::uint32_t>(
                                        signed_window_off +
                                        static_cast<std::int32_t>(static_cast<std::int8_t>(idx)) * gb::TILE_BYTES);
                    decode_tile_8x8(vram, tile_off, pixels.data(), 256, tx * gb::TILE_PIXELS, ty * gb::TILE_PIXELS,
                                    resolver, gbemu::palette_id::bg);
                }
            }
            gbemu::gfx::presenter_upload(c.ppu_bgmap_present, pixels.data());
        }

        void palette_swatch(const char* label, const gbemu::palette_resolver& resolver, gbemu::palette_id pal_id) {
            ImGui::TextUnformatted(label);
            for (int i = 0; i < 4; ++i) {
                const std::uint32_t argb = resolver.resolve(pal_id, static_cast<std::uint8_t>(i));
                const ImVec4 col{((argb >> 16) & 0xFF) / 255.0f, ((argb >> 8) & 0xFF) / 255.0f, (argb & 0xFF) / 255.0f,
                                 1.0f};
                ImGui::SameLine();
                ImGui::PushID(label);
                ImGui::PushID(i);
                ImGui::ColorButton("##swatch", col,
                                   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop |
                                       ImGuiColorEditFlags_NoBorder,
                                   ImVec2(18, 18));
                ImGui::PopID();
                ImGui::PopID();
            }
        }

        void draw_ppu_panel(context& c) {
            if (!c.show_ppu)
                return;
            if (!ImGui::Begin("PPU", &c.show_ppu)) {
                ImGui::End();
                return;
            }

            const auto& mmu = c.core->mmu;
            const auto lcdc = mmu.hwr_lcdc();
            const auto stat = mmu.hwr_stat();

            // Registers block (matches the dump_ppu format so a glance at the
            // panel maps 1:1 to a `dump ppu` from the headless runner).
            ImGui::Text("LCDC=%02X STAT=%02X", lcdc, stat);
            ImGui::Text("SCY=%02X SCX=%02X LY=%02X LYC=%02X", mmu.hwr_scy(), mmu.hwr_scx(), mmu.hwr_ly(),
                        mmu.hwr_lyc());
            ImGui::Text("WY=%02X  WX=%02X", mmu.hwr_wy(), mmu.hwr_wx());

            ImGui::SeparatorText("LCDC");
            ImGui::Text("enable %d  win_map %d  win_en %d  tile_data %d", (lcdc >> 7) & 1, (lcdc >> 6) & 1,
                        (lcdc >> 5) & 1, (lcdc >> 4) & 1);
            ImGui::Text("bg_map %d  obj_size %d  obj_en %d  bg_en %d", (lcdc >> 3) & 1, (lcdc >> 2) & 1,
                        (lcdc >> 1) & 1, lcdc & 1);

            ImGui::SeparatorText("STAT");
            ImGui::Text("lyc_ie %d  m2_ie %d  m1_ie %d  m0_ie %d  coinc %d  mode %d", (stat >> 6) & 1, (stat >> 5) & 1,
                        (stat >> 4) & 1, (stat >> 3) & 1, (stat >> 2) & 1, stat & 3);

            ImGui::SeparatorText("Palettes");
            const auto& resolver = c.core->ppu.palette();
            palette_swatch("BGP ", resolver, gbemu::palette_id::bg);
            palette_swatch("OBP0", resolver, gbemu::palette_id::obj0);
            palette_swatch("OBP1", resolver, gbemu::palette_id::obj1);

            // Heavy-cost viewers refresh only when we're actually drawing the
            // panel — the early-return on `ImGui::Begin(... ) == false` above
            // already gates this for collapsed / unselected-tab cases, so a
            // docked-but-hidden PPU panel stays cheap.  Textures live across
            // hides (only freed in ui::shutdown) so reopening is instant.
            ImGui::SeparatorText("VRAM tiles ($8000-$97FF)");
            refresh_tile_viewer_texture(c);
            if (c.ppu_tiles_present)
                ImGui::Image(gbemu::gfx::presenter_imgui_id(c.ppu_tiles_present), ImVec2(128 * 2, 192 * 2));

            ImGui::SeparatorText("BG map");
            ImGui::RadioButton("$9800", &c.ppu_bgmap_idx, 0);
            ImGui::SameLine();
            ImGui::RadioButton("$9C00", &c.ppu_bgmap_idx, 1);
            refresh_bgmap_viewer_texture(c);
            if (c.ppu_bgmap_present)
                ImGui::Image(gbemu::gfx::presenter_imgui_id(c.ppu_bgmap_present), ImVec2(256, 256));

            ImGui::End();
        }

        void draw_mbc_panel(context& c) {
            if (!c.show_mbc)
                return;
            if (!ImGui::Begin("MBC", &c.show_mbc)) {
                ImGui::End();
                return;
            }

            const auto* cart = c.core->mmu.cart();
            if (!cart) {
                ImGui::TextUnformatted("(no cartridge attached)");
                ImGui::End();
                return;
            }

            const auto st = cart->debug_state();
            ImGui::Text("Type        $%02X", st.type);
            ImGui::Text("ROM bank    %u", static_cast<unsigned>(st.rom_bank));
            ImGui::Text("RAM bank    %u", static_cast<unsigned>(st.ram_bank));
            ImGui::Text("RAM enabled %s", st.ram_enabled ? "yes" : "no");
            ImGui::Text("Mode        %u", static_cast<unsigned>(st.mode));

            ImGui::Separator();
            const std::uint8_t cgb_flag = cart->cgb_flag();
            const char* cgb_label = "DMG";
            switch (classify_cgb_flag(cgb_flag)) {
                case cgb_support::compat:
                    cgb_label = "CGB-compat";
                    break;
                case cgb_support::cgb_only:
                    cgb_label = "CGB-only";
                    break;
                case cgb_support::none:
                    break;
            }
            ImGui::Text("CGB flag    $%02X (%s)", cgb_flag, cgb_label);
            // Header bytes are stable, but surfacing them next to the live
            // banking state saves a trip to the Memory panel.
            ImGui::Text("Header  ROM size $%02X   RAM size $%02X", c.core->mmu.read_u8(0x0148),
                        c.core->mmu.read_u8(0x0149));

            ImGui::End();
        }

        void draw_serial_panel(context& c) {
            if (!c.show_serial)
                return;
            if (!ImGui::Begin("Serial", &c.show_serial)) {
                ImGui::End();
                return;
            }

            auto& dbg = *c.dbg;
            const auto& buf = dbg.serial_buffer();

            if (ImGui::SmallButton("Clear"))
                dbg.serial_clear();
            ImGui::SameLine();
            ImGui::Text("%u bytes", static_cast<unsigned>(buf.size()));

            ImGui::Separator();

            ImGui::BeginChild("##serial_scroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            if (!buf.empty()) {
                // Treat the ring as a single text blob.  We pass begin/end so
                // a buffer without a terminating NUL still renders correctly.
                ImGui::TextUnformatted(buf.data(), buf.data() + buf.size());
            } else {
                ImGui::TextDisabled("(no bytes received)");
            }
            // Auto-scroll to bottom when new data lands at the tail.
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();

            ImGui::End();
        }

        void draw_pc_ring_panel(context& c) {
            if (!c.show_pc_ring)
                return;
            if (!ImGui::Begin("PC ring", &c.show_pc_ring)) {
                ImGui::End();
                return;
            }

            const auto& ring = c.core->pc_ring;
            const std::size_t idx = c.core->pc_idx;
            const std::size_t cap = ring.size();

            ImGui::TextDisabled("Click an entry to jump the Disassembly view");
            ImGui::Separator();

            ImGui::BeginChild("##pc_ring_scroll", ImVec2(0, 0), false);
            // Walk newest → oldest.  pc_idx points at the next *write* slot,
            // so the entry at idx-1 is the most recently recorded PC.
            for (std::size_t i = 0; i < cap; ++i) {
                const std::size_t k = (idx + cap - 1 - i) % cap;
                const std::uint16_t pc = ring[k];
                char label[24];
                std::snprintf(label, sizeof(label), "%3zu: $%04X##pc_ring", i, pc);
                if (ImGui::Selectable(label)) {
                    c.disasm_view_addr = pc;
                    c.disasm_follow_pc = false;
                }
            }
            ImGui::EndChild();

            ImGui::End();
        }

    } // namespace

    context* init(SDL_Window* window, SDL_Renderer* renderer, gfx::backend* backend, debugger& dbg, gbemu::core& c) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // Keyboard nav is intentionally NOT enabled: it would let ImGui
        // claim the arrow keys, Enter, Backspace and Tab — the same keys
        // the emulator wires to the GB joypad (Up/Down/Left/Right, Start,
        // Select). Without nav, ImGui still works fine via mouse; the
        // arrows always reach core.joypad in the main event loop.

        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);

        auto* ctx = new context{};
        ctx->window = window;
        ctx->renderer = renderer;
        ctx->backend = backend;
        ctx->dbg = &dbg;
        ctx->core = &c;
        ctx->display_present = gbemu::gfx::presenter_create(backend, gb::LCD_WIDTH, gb::LCD_HEIGHT);
        load_recent_roms(*ctx);
        return ctx;
    }

    host_actions& actions(context* ctx) {
        return ctx->actions;
    }

    void add_recent_rom(context* ctx, const std::string& path) {
        if (!ctx || path.empty())
            return;
        auto& rec = ctx->recent_roms;
        // Dedup first so the moved-to-front entry doesn't leave a duplicate.
        rec.erase(std::remove(rec.begin(), rec.end(), path), rec.end());
        rec.insert(rec.begin(), path);
        if (rec.size() > MAX_RECENT_ROMS)
            rec.resize(MAX_RECENT_ROMS);
        save_recent_roms(*ctx);
    }

    void apply_display_config(context* ctx, const config& cfg, const std::string& palettes_dir) {
        if (!ctx)
            return;
        // 1) Built-ins first so "grey"/"dmg"/"mgb"/"gbl" are always available
        //    even when the user palettes dir is missing.  User .sbp files
        //    may shadow a built-in by sharing its stem (last-wins).
        ctx->palettes.install_builtins();
        ctx->palettes.scan_directory(palettes_dir);

        // 2) Frame blending mode straight from the config string.
        ctx->post.set_mode(gbemu::display::parse_blend_mode(cfg.display.frame_blending));

        // 3) Resolve the active palette name.  Unknown names fall back to
        //    "grey" rather than throwing — the user has just typed a string
        //    in the config and a typo shouldn't crash the emulator.
        const auto* p = ctx->palettes.find(cfg.display.palette);
        if (!p) {
            LOG_WARNING(gbemu::log::root(), "display: unknown palette \"{}\", falling back to \"grey\"",
                        cfg.display.palette);
            p = ctx->palettes.find("grey");
        }
        if (p && ctx->core) {
            ctx->core->ppu.set_palette(p->shades);
            ctx->active_palette_name = p->name;
        }
    }

    void reset_display_post(context* ctx) {
        if (!ctx)
            return;
        ctx->post.reset();
    }

    void shutdown(context* ctx) {
        if (!ctx)
            return;
        gbemu::gfx::presenter_destroy(ctx->ppu_tiles_present);
        ctx->ppu_tiles_present = nullptr;
        gbemu::gfx::presenter_destroy(ctx->ppu_bgmap_present);
        ctx->ppu_bgmap_present = nullptr;
        gbemu::gfx::presenter_destroy(ctx->display_present);
        ctx->display_present = nullptr;
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        delete ctx;
    }

    bool process_event(context* /*ctx*/, const SDL_Event& e) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        const ImGuiIO& io = ImGui::GetIO();
        // Only treat the event as "captured by ImGui" for keyboard input
        // when the user is actively editing an InputText (hex address in
        // Breakpoints, "Go to" in Disassembly, etc.) — i.e. WantTextInput,
        // not the broader WantCaptureKeyboard. The latter also flips true
        // whenever any window has focus, which would swallow the GB joypad
        // keys for the entire session. Mouse events follow the usual
        // WantCaptureMouse rule so panels eat clicks/hover.
        const bool is_kb = (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP || e.type == SDL_TEXTINPUT);
        if (is_kb)
            return io.WantTextInput;
        return io.WantCaptureMouse;
    }

    void render_frame(context* ctx) {
        if (!ctx)
            return;

        // Refresh the GB framebuffer presenter on every new frame-ready edge
        // from the PPU.  When paused, no edge fires and the Display panel
        // keeps showing the last produced frame.  Previously this lived in
        // Application::run; consolidating it here lets the UI fully own the
        // display presenter's lifecycle.
        if (ctx->display_present && ctx->core && ctx->core->ppu.consume_frame_ready()) {
            // Frame blending: with mode == disabled the blender returns
            // `framebuffer()` verbatim (zero copy) so this stays cheap on
            // the default path.  Other modes mix in the history ring.
            const std::uint32_t* fb = ctx->post.blend(ctx->core->ppu.framebuffer());
            gbemu::gfx::presenter_upload(ctx->display_present, fb);
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        setup_dockspace_and_menubar(*ctx);
        draw_display_panel(*ctx);
        draw_cpu_panel(*ctx);
        draw_disasm_panel(*ctx);
        draw_memory_panel(*ctx);
        draw_breakpoints_panel(*ctx);
        draw_ppu_panel(*ctx);
        draw_mbc_panel(*ctx);
        draw_serial_panel(*ctx);
        draw_pc_ring_panel(*ctx);

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ctx->renderer);
    }

} // namespace gbemu::ui
