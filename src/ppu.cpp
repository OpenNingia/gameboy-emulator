#include <algorithm>

#include <ppu.h>

using namespace gbemu;

ppu::ppu(mmu& m) : m(m) {}

void ppu::step(std::uint32_t t_cycles) {
    const auto lcdc = m.hwr_lcdc();
    if (!(lcdc & 0x80)) {
        // LCD off: reset to known state. STAT mode bits read as 0 while the LCD
        // is off; internally we park at OAM_SCAN/line 0 so re-enable resumes
        // cleanly via the lcd_was_off branch below.
        dots_in_mode = 0;
        mode = mode_e::OAM_SCAN;
        m.hwr_ly(0);
        m.hwr_stat(m.hwr_stat() & 0xFC);
        lcd_was_off = true;
        window_triggered = false;
        window_line = 0;
        return;
    }

    if (lcd_was_off) {
        lcd_was_off = false;
        enter_oam_scan();
    }

    advance(t_cycles);
}

void ppu::advance(std::uint32_t t_cycles) {
    while (t_cycles > 0) {
        const auto remaining = dots_for(mode) - dots_in_mode;
        const auto consume = std::min<std::uint32_t>(t_cycles, remaining);
        dots_in_mode += consume;
        t_cycles -= consume;

        if (dots_in_mode < dots_for(mode))
            continue;

        dots_in_mode = 0;
        switch (mode) {
            case mode_e::OAM_SCAN:
                enter_drawing();
                break;
            case mode_e::DRAWING:
                enter_hblank();
                break;
            case mode_e::HBLANK:
                next_line();
                break;
            case mode_e::VBLANK:
                next_line();
                break;
        }
    }
}

std::uint32_t ppu::dots_for(mode_e mode) const {
    switch (mode) {
        case mode_e::OAM_SCAN:
            return OAM_SCAN_DOTS;
        case mode_e::DRAWING:
            return DRAWING_DOTS;
        case mode_e::HBLANK:
            return HBLANK_DOTS;
        case mode_e::VBLANK:
            return SCANLINE_DOTS; // step one VBlank scanline at a time
    }
    return 0;
}

void ppu::enter_oam_scan() {
    set_stat_mode(mode_e::OAM_SCAN);
    if (m.hwr_stat() & 0x20) // STAT mode-2 IRQ source
        request_irq(1);
}

void ppu::enter_drawing() {
    set_stat_mode(mode_e::DRAWING);
    // No STAT IRQ source for mode 3.
}

void ppu::enter_hblank() {
    // Drawing just finished — push the line to the framebuffer.
    const auto ly = m.hwr_ly();
    render_bg_scanline(ly);
    render_window_scanline(ly); // overlays BG and updates bg_color_line for sprite priority
    render_sprites_scanline(ly);
    set_stat_mode(mode_e::HBLANK);
    if (m.hwr_stat() & 0x08) // STAT mode-0 IRQ source
        request_irq(1);
}

void ppu::enter_vblank() {
    set_stat_mode(mode_e::VBLANK);
    request_irq(0);          // VBlank → IF.0
    if (m.hwr_stat() & 0x10) // STAT mode-1 IRQ source
        request_irq(1);
    frame_ready = true;
}

void ppu::next_line() {
    const std::uint8_t ly = (m.hwr_ly() + 1) % TOTAL_LINES;
    m.hwr_ly(ly);
    update_lyc_coincidence();

    if (ly == 0) {
        // Frame boundary — drop the per-frame window latch so WY is re-evaluated
        // from scratch and the internal window line counter restarts at 0.
        window_triggered = false;
        window_line = 0;
    }

    if (ly == VISIBLE_LINES)
        enter_vblank();
    else if (ly < VISIBLE_LINES)
        enter_oam_scan();
    // ly in [145, 153]: stay in VBLANK — mode unchanged, just consume another scanline.
}

void ppu::set_stat_mode(mode_e new_mode) {
    mode = new_mode;
    m.hwr_stat((m.hwr_stat() & 0xFC) | static_cast<std::uint8_t>(new_mode));
}

void ppu::render_bg_scanline(std::uint8_t ly) {
    static constexpr std::array<std::uint32_t, 4> palette = {0xFFFFFFFFu, 0xFFAAAAAAu, 0xFF555555u, 0xFF000000u};

    const auto lcdc = m.hwr_lcdc();
    const auto scy = m.hwr_scy();
    const auto scx = m.hwr_scx();
    const auto bgp = m.hwr_bgp();

    // LCDC.0 = BG enable. If off, fill scanline with shade 0 of BGP and treat
    // BG as transparent (color 0) for sprite priority so OBJs always show through.
    if (!(lcdc & 0x01)) {
        const auto bg_off = palette[bgp & 0x03];
        for (int px = 0; px < 160; ++px) {
            fb[ly * 160 + px] = bg_off;
            bg_color_line[px] = 0;
        }
        return;
    }

    const std::uint16_t map_base = (lcdc & 0x08) ? 0x9C00 : 0x9800;
    const bool data_8000 = (lcdc & 0x10) != 0;

    const std::uint8_t y = static_cast<std::uint8_t>(ly + scy);
    const std::uint8_t tile_y = y >> 3;
    const std::uint8_t row = y & 7;

    for (int px = 0; px < 160; ++px) {
        const std::uint8_t x = static_cast<std::uint8_t>(px + scx);
        const std::uint8_t tile_x = x >> 3;
        const std::uint8_t col = 7 - (x & 7);

        const std::uint8_t idx = m.vram[map_base - 0x8000 + tile_y * 32 + tile_x];

        const std::uint16_t data_addr =
            data_8000 ? static_cast<std::uint16_t>(0x8000 + idx * 16 + row * 2)
                      : static_cast<std::uint16_t>(0x9000 + static_cast<std::int8_t>(idx) * 16 + row * 2);

        const std::uint8_t lo = m.vram[data_addr - 0x8000];
        const std::uint8_t hi = m.vram[data_addr - 0x8000 + 1];

        const std::uint8_t ci = static_cast<std::uint8_t>((((hi >> col) & 1) << 1) | ((lo >> col) & 1));
        const std::uint8_t shade = (bgp >> (ci * 2)) & 0x03;

        fb[ly * 160 + px] = palette[shade];
        bg_color_line[px] = ci;
    }
}

void ppu::render_window_scanline(std::uint8_t ly) {
    static constexpr std::array<std::uint32_t, 4> palette = {0xFFFFFFFFu, 0xFFAAAAAAu, 0xFF555555u, 0xFF000000u};

    // The WY==LY latch is unconditional — it arms even if LCDC.5 is off at that
    // moment, so the window can pop in mid-frame when the game flips LCDC.5 on.
    if (ly == m.hwr_wy())
        window_triggered = true;

    const auto lcdc = m.hwr_lcdc();
    // DMG: LCDC.0 gates BG *and* window. LCDC.5 enables the window itself.
    if (!(lcdc & 0x01) || !(lcdc & 0x20))
        return;
    if (!window_triggered)
        return;

    const auto wx = m.hwr_wx();
    const int xstart = static_cast<int>(wx) - 7; // screen X where the window's column 0 lands
    if (xstart >= 160)
        return; // entirely off-screen to the right — internal counter does NOT advance

    const std::uint16_t map_base = (lcdc & 0x40) ? 0x9C00 : 0x9800; // LCDC.6 = window map
    const bool data_8000 = (lcdc & 0x10) != 0;
    const auto bgp = m.hwr_bgp();

    const std::uint8_t y = window_line;
    const std::uint8_t tile_y = y >> 3;
    const std::uint8_t row = y & 7;

    const int px_start = (xstart < 0) ? 0 : xstart;
    for (int px = px_start; px < 160; ++px) {
        const int win_x = px - xstart; // window-space X (always >= 0 here)
        const std::uint8_t tile_x = static_cast<std::uint8_t>(win_x >> 3);
        const std::uint8_t col = 7 - (win_x & 7);

        const std::uint8_t idx = m.vram[map_base - 0x8000 + tile_y * 32 + tile_x];

        const std::uint16_t data_addr =
            data_8000 ? static_cast<std::uint16_t>(0x8000 + idx * 16 + row * 2)
                      : static_cast<std::uint16_t>(0x9000 + static_cast<std::int8_t>(idx) * 16 + row * 2);

        const std::uint8_t lo = m.vram[data_addr - 0x8000];
        const std::uint8_t hi = m.vram[data_addr - 0x8000 + 1];

        const std::uint8_t ci = static_cast<std::uint8_t>((((hi >> col) & 1) << 1) | ((lo >> col) & 1));
        const std::uint8_t shade = (bgp >> (ci * 2)) & 0x03;

        fb[ly * 160 + px] = palette[shade];
        bg_color_line[px] = ci; // sprites use this for the BG-priority bit
    }

    // At least one window pixel was emitted this scanline → advance the latch.
    window_line++;
}

void ppu::render_sprites_scanline(std::uint8_t ly) {
    static constexpr std::array<std::uint32_t, 4> palette = {0xFFFFFFFFu, 0xFFAAAAAAu, 0xFF555555u, 0xFF000000u};

    const auto lcdc = m.hwr_lcdc();
    if (!(lcdc & 0x02)) // LCDC.1 = OBJ enable
        return;

    const int height = (lcdc & 0x04) ? 16 : 8; // LCDC.2 = OBJ size

    // OAM scan — collect up to 10 sprites whose vertical range covers ly.
    struct sprite_entry {
        std::uint8_t y, x, tile, attr;
        std::uint8_t oam_idx; // stable tie-breaker for the DMG priority sort
    };
    std::array<sprite_entry, 10> visible{};
    std::size_t n_visible = 0;

    for (std::uint8_t i = 0; i < 40 && n_visible < 10; ++i) {
        const std::uint8_t y = m.sram[i * 4 + 0];
        const std::uint8_t x = m.sram[i * 4 + 1];
        const int sprite_top = static_cast<int>(y) - 16;
        if (static_cast<int>(ly) >= sprite_top && static_cast<int>(ly) < sprite_top + height) {
            visible[n_visible++] = {y, x, m.sram[i * 4 + 2], m.sram[i * 4 + 3], i};
        }
    }

    // DMG sprite priority: smaller X wins; ties go to the earlier OAM entry.
    // stable_sort preserves the OAM order for equal X.
    std::stable_sort(visible.begin(), visible.begin() + n_visible,
                     [](const sprite_entry& a, const sprite_entry& b) { return a.x < b.x; });

    // Draw in priority order with a per-pixel "claimed" mask so lower-priority
    // sprites cannot poke through a higher-priority sprite's opaque pixels —
    // even when that pixel lost to BG via the BG-priority bit.
    std::array<bool, 160> claimed{};

    for (std::size_t s = 0; s < n_visible; ++s) {
        const auto& sp = visible[s];
        const int sprite_top = static_cast<int>(sp.y) - 16;
        const int sprite_left = static_cast<int>(sp.x) - 8;

        const bool y_flip = (sp.attr & 0x40) != 0;
        const bool x_flip = (sp.attr & 0x20) != 0;
        const bool bg_priority = (sp.attr & 0x80) != 0;
        const std::uint8_t pal = (sp.attr & 0x10) ? m.hwr_obp1() : m.hwr_obp0();

        int row = static_cast<int>(ly) - sprite_top;
        if (y_flip)
            row = (height - 1) - row;

        // In 8x16 mode bit 0 of the tile index is ignored; the row offset
        // naturally walks into tile N+1 when row >= 8 because consecutive
        // tile rows in VRAM are contiguous.
        std::uint8_t tile_index = sp.tile;
        if (height == 16)
            tile_index &= 0xFE;

        // Sprites always use $8000-unsigned addressing regardless of LCDC.4.
        const std::uint16_t data_addr = static_cast<std::uint16_t>(0x8000 + tile_index * 16 + row * 2);
        const std::uint8_t lo = m.vram[data_addr - 0x8000];
        const std::uint8_t hi = m.vram[data_addr - 0x8000 + 1];

        for (int px = 0; px < 8; ++px) {
            const int screen_x = sprite_left + px;
            if (screen_x < 0 || screen_x >= 160)
                continue;
            if (claimed[screen_x])
                continue;

            const int bit = x_flip ? px : (7 - px);
            const std::uint8_t ci = static_cast<std::uint8_t>((((hi >> bit) & 1) << 1) | ((lo >> bit) & 1));
            if (ci == 0)
                continue; // sprite color 0 = transparent (no claim)

            claimed[screen_x] = true;
            if (bg_priority && bg_color_line[screen_x] != 0)
                continue; // OBJ behind BG colors 1-3

            const std::uint8_t shade = (pal >> (ci * 2)) & 0x03;
            fb[ly * 160 + screen_x] = palette[shade];
        }
    }
}

void ppu::update_lyc_coincidence() {
    const bool coinc = (m.hwr_ly() == m.hwr_lyc());
    m.hwr_stat((m.hwr_stat() & ~0x04) | (coinc ? 0x04 : 0x00));
    if (coinc && (m.hwr_stat() & 0x40))
        request_irq(1); // STAT → IF.1
}

void ppu::request_irq(std::uint8_t bit) {
    m.hwr_if(m.hwr_if() | (1 << bit));
}

bool ppu::consume_frame_ready() {
    bool was_ready = frame_ready;
    frame_ready = false;
    return was_ready;
}
