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
#include <string>

#include <SDL2/SDL.h>
#include <core.h>
#include <debugger.h>
#include <disasm.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_internal.h>
#include <mbc.h>
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

        // PPU panel: streaming textures for the VRAM tile grid (128x192 px,
        // 16x24 tiles of 8x8) and the BG tile map viewer (256x256 px).  Both
        // are created lazily on first use and recycled across frames.  The
        // map_idx flag selects which of the two BG maps to view ($9800 vs
        // $9C00); independent of LCDC.3 so the user can inspect either.
        SDL_Texture* ppu_tiles_tex{nullptr};
        SDL_Texture* ppu_bgmap_tex{nullptr};
        int ppu_bgmap_idx{0}; // 0 → $9800, 1 → $9C00
    };

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
                    ImGui::MenuItem("PPU", nullptr, &c.show_ppu);
                    ImGui::MenuItem("MBC", nullptr, &c.show_mbc);
                    ImGui::MenuItem("Serial", nullptr, &c.show_serial);
                    ImGui::MenuItem("PC ring", nullptr, &c.show_pc_ring);
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
                // Stub: identical to Step.  Proper step-over (run to the
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

        // ---------- PR5: PPU / MBC / Serial / PC-ring ----------

        // Shared DMG four-shade palette used by the BGP/OBP swatches and the
        // tile / map viewers.  Matches the ARGB constants in ppu.cpp.
        constexpr std::array<std::uint32_t, 4> kDmgPalette = {0xFFFFFFFFu, 0xFFAAAAAAu, 0xFF555555u, 0xFF000000u};

        // Decode an 8x8 GB tile (16 bytes at vram_offset) into `out` (a
        // row-major buffer of dst_stride pixels per row), placing the
        // top-left corner of the tile at (dst_x, dst_y).  Each output pixel
        // is ARGB8888 driven by `palette` (BGP-mapped).
        void decode_tile_8x8(const std::uint8_t* vram, std::uint32_t vram_offset, std::uint32_t* out,
                             std::uint32_t dst_stride, std::uint32_t dst_x, std::uint32_t dst_y,
                             const std::uint8_t bgp) {
            for (std::uint32_t row = 0; row < 8; ++row) {
                const std::uint8_t lo = vram[vram_offset + row * 2];
                const std::uint8_t hi = vram[vram_offset + row * 2 + 1];
                for (std::uint32_t col = 0; col < 8; ++col) {
                    const std::uint8_t shift = static_cast<std::uint8_t>(7 - col);
                    const std::uint8_t ci = static_cast<std::uint8_t>((((hi >> shift) & 1) << 1) | ((lo >> shift) & 1));
                    const std::uint8_t shade = (bgp >> (ci * 2)) & 0x03;
                    out[(dst_y + row) * dst_stride + (dst_x + col)] = kDmgPalette[shade];
                }
            }
        }

        // Repaint the 16x24 tile-data grid covering VRAM $8000-$97FF.  Tiles
        // are laid out row-major: index 0 top-left, index 15 top-right,
        // index 16 second row, etc.  Result is uploaded into ppu_tiles_tex.
        void refresh_tile_viewer_texture(context& c) {
            if (!c.ppu_tiles_tex) {
                c.ppu_tiles_tex =
                    SDL_CreateTexture(c.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 128, 192);
                if (!c.ppu_tiles_tex)
                    return;
            }
            std::array<std::uint32_t, 128 * 192> pixels{};
            const std::uint8_t bgp = c.core->mmu.hwr_bgp();
            const std::uint8_t* vram = c.core->mmu.vram.data();
            for (std::uint32_t t = 0; t < 384; ++t) {
                const std::uint32_t tx = (t % 16) * 8;
                const std::uint32_t ty = (t / 16) * 8;
                decode_tile_8x8(vram, t * 16, pixels.data(), 128, tx, ty, bgp);
            }
            SDL_UpdateTexture(c.ppu_tiles_tex, nullptr, pixels.data(), 128 * 4);
        }

        // Repaint a 256x256 BG tile-map viewer.  Reads tile indices from the
        // map area selected by c.ppu_bgmap_idx ($9800 vs $9C00) and resolves
        // tile data through the current LCDC.4 addressing mode (the same
        // logic ppu::render_bg_scanline uses).
        void refresh_bgmap_viewer_texture(context& c) {
            if (!c.ppu_bgmap_tex) {
                c.ppu_bgmap_tex =
                    SDL_CreateTexture(c.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 256);
                if (!c.ppu_bgmap_tex)
                    return;
            }
            std::array<std::uint32_t, 256 * 256> pixels{};
            const std::uint8_t lcdc = c.core->mmu.hwr_lcdc();
            const std::uint8_t bgp = c.core->mmu.hwr_bgp();
            const bool data_8000 = (lcdc & 0x10) != 0;
            const std::uint16_t map_base = c.ppu_bgmap_idx ? 0x9C00 : 0x9800;
            const std::uint8_t* vram = c.core->mmu.vram.data();
            for (std::uint32_t ty = 0; ty < 32; ++ty) {
                for (std::uint32_t tx = 0; tx < 32; ++tx) {
                    const std::uint8_t idx = vram[(map_base - 0x8000) + ty * 32 + tx];
                    const std::uint32_t tile_off =
                        data_8000 ? static_cast<std::uint32_t>(idx) * 16u
                                  : static_cast<std::uint32_t>(
                                        0x1000 + static_cast<std::int32_t>(static_cast<std::int8_t>(idx)) * 16);
                    decode_tile_8x8(vram, tile_off, pixels.data(), 256, tx * 8, ty * 8, bgp);
                }
            }
            SDL_UpdateTexture(c.ppu_bgmap_tex, nullptr, pixels.data(), 256 * 4);
        }

        void palette_swatch(const char* label, std::uint8_t pal) {
            ImGui::TextUnformatted(label);
            for (int i = 0; i < 4; ++i) {
                const std::uint8_t shade = (pal >> (i * 2)) & 0x03;
                const std::uint32_t argb = kDmgPalette[shade];
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
            palette_swatch("BGP ", mmu.hwr_bgp());
            palette_swatch("OBP0", mmu.hwr_obp0());
            palette_swatch("OBP1", mmu.hwr_obp1());

            // Heavy-cost viewers refresh only when we're actually drawing the
            // panel — the early-return on `ImGui::Begin(... ) == false` above
            // already gates this for collapsed / unselected-tab cases, so a
            // docked-but-hidden PPU panel stays cheap.  Textures live across
            // hides (only freed in ui::shutdown) so reopening is instant.
            ImGui::SeparatorText("VRAM tiles ($8000-$97FF)");
            refresh_tile_viewer_texture(c);
            if (c.ppu_tiles_tex)
                ImGui::Image(reinterpret_cast<ImTextureID>(c.ppu_tiles_tex), ImVec2(128 * 2, 192 * 2));

            ImGui::SeparatorText("BG map");
            ImGui::RadioButton("$9800", &c.ppu_bgmap_idx, 0);
            ImGui::SameLine();
            ImGui::RadioButton("$9C00", &c.ppu_bgmap_idx, 1);
            refresh_bgmap_viewer_texture(c);
            if (c.ppu_bgmap_tex)
                ImGui::Image(reinterpret_cast<ImTextureID>(c.ppu_bgmap_tex), ImVec2(256, 256));

            ImGui::End();
        }

        void draw_mbc_panel(context& c) {
            if (!c.show_mbc)
                return;
            if (!ImGui::Begin("MBC", &c.show_mbc)) {
                ImGui::End();
                return;
            }

            if (!c.core->mmu.cart) {
                ImGui::TextUnformatted("(no cartridge attached)");
                ImGui::End();
                return;
            }

            const auto st = c.core->mmu.cart->debug_state();
            ImGui::Text("Type        $%02X", st.type);
            ImGui::Text("ROM bank    %u", static_cast<unsigned>(st.rom_bank));
            ImGui::Text("RAM bank    %u", static_cast<unsigned>(st.ram_bank));
            ImGui::Text("RAM enabled %s", st.ram_enabled ? "yes" : "no");
            ImGui::Text("Mode        %u", static_cast<unsigned>(st.mode));

            ImGui::Separator();
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

    context* init(SDL_Window* window, SDL_Renderer* renderer, debugger& dbg, gbemu::core& c) {
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
        ctx->dbg = &dbg;
        ctx->core = &c;
        return ctx;
    }

    void shutdown(context* ctx) {
        if (!ctx)
            return;
        if (ctx->ppu_tiles_tex) {
            SDL_DestroyTexture(ctx->ppu_tiles_tex);
            ctx->ppu_tiles_tex = nullptr;
        }
        if (ctx->ppu_bgmap_tex) {
            SDL_DestroyTexture(ctx->ppu_bgmap_tex);
            ctx->ppu_bgmap_tex = nullptr;
        }
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
        draw_ppu_panel(*ctx);
        draw_mbc_panel(*ctx);
        draw_serial_panel(*ctx);
        draw_pc_ring_panel(*ctx);

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ctx->renderer);
    }

} // namespace gbemu::ui
