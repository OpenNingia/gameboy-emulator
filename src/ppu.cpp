#include <algorithm>

#include <gb_layout.h>
#include <irq.h>
#include <pixel_pipeline.h>
#include <ppu.h>

using namespace gbemu;

ppu::ppu(mmu& m, irq& i) : mmu_(m), irq_(i), resolver_(m) {}

void ppu::step(std::uint32_t t_cycles) {
    const auto lcdc = mmu_.hwr_lcdc();
    if (!(lcdc & gb::lcdc::lcd_enable)) {
        // LCD off: reset to known state. STAT mode bits read as 0 while the LCD
        // is off; internally we park at OAM_SCAN/line 0 so re-enable resumes
        // cleanly via the lcd_was_off branch below.
        dots_in_mode = 0;
        mode = mode_e::OAM_SCAN;
        mmu_.hwr_ly(0);
        mmu_.hwr_stat(mmu_.hwr_stat() & ~gb::stat::mode_mask);
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
            return gb::ppu_timing::OAM_SCAN_DOTS;
        case mode_e::DRAWING:
            return gb::ppu_timing::DRAWING_DOTS;
        case mode_e::HBLANK:
            return gb::ppu_timing::HBLANK_DOTS;
        case mode_e::VBLANK:
            return gb::ppu_timing::SCANLINE_DOTS; // step one VBlank scanline at a time
    }
    return 0;
}

void ppu::enter_oam_scan() {
    set_stat_mode(mode_e::OAM_SCAN);
    if (mmu_.hwr_stat() & gb::stat::mode2_irq_enable)
        irq_.request(irq::source::lcd_stat);
}

void ppu::enter_drawing() {
    set_stat_mode(mode_e::DRAWING);
    // No STAT IRQ source for mode 3.
}

void ppu::enter_hblank() {
    // Drawing just finished — push the line to the framebuffer.
    const auto ly = mmu_.hwr_ly();
    render_bg_scanline(ly);
    render_window_scanline(ly); // overlays BG and updates bg_attr_line for sprite priority
    render_sprites_scanline(ly);
    set_stat_mode(mode_e::HBLANK);
    if (mmu_.hwr_stat() & gb::stat::mode0_irq_enable)
        irq_.request(irq::source::lcd_stat);
}

void ppu::enter_vblank() {
    set_stat_mode(mode_e::VBLANK);
    irq_.request(irq::source::vblank);
    if (mmu_.hwr_stat() & gb::stat::mode1_irq_enable)
        irq_.request(irq::source::lcd_stat);
    frame_ready = true;
}

void ppu::next_line() {
    const std::uint8_t ly = (mmu_.hwr_ly() + 1) % gb::ppu_timing::TOTAL_LINES;
    mmu_.hwr_ly(ly);
    update_lyc_coincidence();

    if (ly == 0) {
        // Frame boundary — drop the per-frame window latch so WY is re-evaluated
        // from scratch and the internal window line counter restarts at 0.
        window_triggered = false;
        window_line = 0;
    }

    if (ly == gb::ppu_timing::VISIBLE_LINES)
        enter_vblank();
    else if (ly < gb::ppu_timing::VISIBLE_LINES)
        enter_oam_scan();
    // ly in [145, 153]: stay in VBLANK — mode unchanged, just consume another scanline.
}

void ppu::set_stat_mode(mode_e new_mode) {
    mode = new_mode;
    mmu_.hwr_stat((mmu_.hwr_stat() & ~gb::stat::mode_mask) | static_cast<std::uint8_t>(new_mode));
}

namespace {
    // Compute the VRAM byte address of `row` of the tile at `tile_index`,
    // honoring LCDC.4 addressing (true = $8000-unsigned, false = $9000-signed).
    inline std::uint16_t bg_tile_row_addr(std::uint8_t tile_index, std::uint8_t row, bool data_8000) {
        if (data_8000)
            return static_cast<std::uint16_t>(gb::TILE_DATA_UNSIGNED_BASE + tile_index * gb::TILE_BYTES + row * 2);
        return static_cast<std::uint16_t>(gb::TILE_DATA_SIGNED_BASE +
                                          static_cast<std::int8_t>(tile_index) * gb::TILE_BYTES + row * 2);
    }

    // Decode the CGB BG attribute byte (VRAM bank 1, same map offset as the
    // tile index byte in bank 0) into a pixel_attr. The five honored bits
    // are bits 0-2 (palette 0-7 → cgb_bgN), bit 3 (tile-data VRAM bank),
    // bit 5 (x-flip), bit 6 (y-flip), bit 7 (BG-over-OBJ priority).
    inline gbemu::pixel_attr decode_bg_attr_byte(std::uint8_t b) {
        return {
            static_cast<gbemu::palette_id>(static_cast<std::uint8_t>(gbemu::palette_id::cgb_bg0) +
                                           (b & gb::bg_attr::cgb_palette_mask)),
            static_cast<std::uint8_t>((b & gb::bg_attr::cgb_vram_bank) ? 1 : 0),
            (b & gb::bg_attr::priority) != 0,
            (b & gb::bg_attr::x_flip) != 0,
            (b & gb::bg_attr::y_flip) != 0,
        };
    }
} // namespace

void ppu::render_bg_scanline(std::uint8_t ly) {
    const auto lcdc = mmu_.hwr_lcdc();
    const auto scy = mmu_.hwr_scy();
    const auto scx = mmu_.hwr_scx();

    // LCDC.0 has different semantics across models:
    //   DMG: BG enable.  When 0, the BG layer is blanked to shade 0 of BGP
    //        and presented as transparent (color_index 0) so OBJs show through.
    //   CGB: BG/window master priority. When 0, BG and window are still
    //        drawn normally — they just lose their priority gate against
    //        OBJs (the sprite-stage gating below keys off LCDC.0 directly).
    // So only the DMG path takes the blank-and-return shortcut.
    if (!mmu_.cgb_mode() && !(lcdc & gb::lcdc::bg_enable)) {
        const std::uint32_t bg_off = resolve_pixel(palette_id::bg, 0);
        for (int px = 0; px < gb::LCD_WIDTH; ++px) {
            fb[ly * gb::LCD_WIDTH + px] = bg_off;
            bg_attr_line[px] = {0, DMG_BG_ATTR};
        }
        return;
    }

    const std::uint16_t map_base = (lcdc & gb::lcdc::bg_map_9c00) ? gb::BG_MAP_1 : gb::BG_MAP_0;
    const bool data_8000 = (lcdc & gb::lcdc::tile_data_8000) != 0;

    const std::uint8_t y = static_cast<std::uint8_t>(ly + scy);
    const std::uint8_t tile_y = y >> 3;

    for (int px = 0; px < gb::LCD_WIDTH; ++px) {
        const std::uint8_t x = static_cast<std::uint8_t>(px + scx);
        const std::uint8_t tile_x = x >> 3;

        const std::uint16_t map_off =
            static_cast<std::uint16_t>(map_base - gb::VRAM_BASE + tile_y * gb::TILES_PER_MAP_ROW + tile_x);
        const std::uint8_t idx = mmu_.vram_read(map_off);

        // DMG: every BG/window tile uses the same default attribute. CGB:
        // the attribute byte lives at the same map_off in VRAM bank 1 and
        // carries palette (0-2), tile-data bank (3), x-flip (5), y-flip (6),
        // BG-over-OBJ priority (7).
        const pixel_attr attr = mmu_.cgb_mode() ? decode_bg_attr_byte(mmu_.vram_read(map_off, 1)) : DMG_BG_ATTR;

        const std::uint8_t row =
            attr.y_flip ? static_cast<std::uint8_t>(7 - (y & 7)) : static_cast<std::uint8_t>(y & 7);
        const std::uint8_t col =
            attr.x_flip ? static_cast<std::uint8_t>(x & 7) : static_cast<std::uint8_t>(7 - (x & 7));

        const std::uint16_t data_addr = bg_tile_row_addr(idx, row, data_8000);
        const std::uint8_t lo = mmu_.vram_read(static_cast<std::uint16_t>(data_addr - gb::VRAM_BASE), attr.bank);
        const std::uint8_t hi = mmu_.vram_read(static_cast<std::uint16_t>(data_addr - gb::VRAM_BASE + 1), attr.bank);

        // (color_index, attr) is the pipeline intermediate; resolve_pixel
        // dispatches attr.id + color_index through the DMG or CGB resolver
        // based on the cartridge model.
        const std::uint8_t ci = tile_color_index(lo, hi, col);
        fb[ly * gb::LCD_WIDTH + px] = resolve_pixel(attr.id, ci);
        bg_attr_line[px] = {ci, attr};
    }
}

void ppu::render_window_scanline(std::uint8_t ly) {
    // The WY==LY latch is unconditional — it arms even if LCDC.5 is off at that
    // moment, so the window can pop in mid-frame when the game flips LCDC.5 on.
    if (ly == mmu_.hwr_wy())
        window_triggered = true;

    const auto lcdc = mmu_.hwr_lcdc();
    // LCDC.5 enables the window itself on both models. LCDC.0 also gates the
    // window on DMG (it's the unified BG/window enable there); on CGB LCDC.0
    // is the master-priority bit and does not disable rendering — the window
    // keeps drawing, just without priority over OBJs.
    if (!(lcdc & gb::lcdc::window_enable))
        return;
    if (!mmu_.cgb_mode() && !(lcdc & gb::lcdc::bg_enable))
        return;
    if (!window_triggered)
        return;

    const auto wx = mmu_.hwr_wx();
    const int xstart = static_cast<int>(wx) - 7; // screen X where the window's column 0 lands
    if (xstart >= gb::LCD_WIDTH)
        return; // entirely off-screen to the right — internal counter does NOT advance

    const std::uint16_t map_base = (lcdc & gb::lcdc::window_map_9c00) ? gb::BG_MAP_1 : gb::BG_MAP_0;
    const bool data_8000 = (lcdc & gb::lcdc::tile_data_8000) != 0;

    const std::uint8_t y = window_line;
    const std::uint8_t tile_y = y >> 3;

    const int px_start = (xstart < 0) ? 0 : xstart;
    for (int px = px_start; px < gb::LCD_WIDTH; ++px) {
        const int win_x = px - xstart; // window-space X (always >= 0 here)
        const std::uint8_t tile_x = static_cast<std::uint8_t>(win_x >> 3);

        const std::uint16_t map_off =
            static_cast<std::uint16_t>(map_base - gb::VRAM_BASE + tile_y * gb::TILES_PER_MAP_ROW + tile_x);
        const std::uint8_t idx = mmu_.vram_read(map_off);

        // Same CGB attribute decode as render_bg_scanline (window shares the
        // tile-map attribute layout in VRAM bank 1).
        const pixel_attr attr = mmu_.cgb_mode() ? decode_bg_attr_byte(mmu_.vram_read(map_off, 1)) : DMG_BG_ATTR;

        const std::uint8_t row =
            attr.y_flip ? static_cast<std::uint8_t>(7 - (y & 7)) : static_cast<std::uint8_t>(y & 7);
        const std::uint8_t col =
            attr.x_flip ? static_cast<std::uint8_t>(win_x & 7) : static_cast<std::uint8_t>(7 - (win_x & 7));

        const std::uint16_t data_addr = bg_tile_row_addr(idx, row, data_8000);
        const std::uint8_t lo = mmu_.vram_read(static_cast<std::uint16_t>(data_addr - gb::VRAM_BASE), attr.bank);
        const std::uint8_t hi = mmu_.vram_read(static_cast<std::uint16_t>(data_addr - gb::VRAM_BASE + 1), attr.bank);

        const std::uint8_t ci = tile_color_index(lo, hi, col);
        fb[ly * gb::LCD_WIDTH + px] = resolve_pixel(attr.id, ci);
        bg_attr_line[px] = {ci, attr};
    }

    // At least one window pixel was emitted this scanline → advance the latch.
    window_line++;
}

void ppu::render_sprites_scanline(std::uint8_t ly) {
    const auto lcdc = mmu_.hwr_lcdc();
    if (!(lcdc & gb::lcdc::obj_enable))
        return;

    const int height = (lcdc & gb::lcdc::obj_size_8x16) ? 16 : 8;

    // OAM scan — collect up to SPRITES_PER_LINE sprites whose vertical range covers ly.
    struct sprite_entry {
        std::uint8_t y, x, tile, attr;
        std::uint8_t oam_idx; // stable tie-breaker for the DMG priority sort
    };
    std::array<sprite_entry, gb::SPRITES_PER_LINE> visible{};
    std::size_t n_visible = 0;

    for (std::uint8_t i = 0; i < gb::OAM_ENTRIES && n_visible < gb::SPRITES_PER_LINE; ++i) {
        const std::uint8_t y = mmu_.oam_read(i * gb::OAM_BYTES_PER_ENTRY + 0);
        const std::uint8_t x = mmu_.oam_read(i * gb::OAM_BYTES_PER_ENTRY + 1);
        const int sprite_top = static_cast<int>(y) - 16;
        if (static_cast<int>(ly) >= sprite_top && static_cast<int>(ly) < sprite_top + height) {
            visible[n_visible++] = {y, x, mmu_.oam_read(i * gb::OAM_BYTES_PER_ENTRY + 2),
                                    mmu_.oam_read(i * gb::OAM_BYTES_PER_ENTRY + 3), i};
        }
    }

    // Sprite priority depends on the hardware model:
    //   DMG: smaller X wins; equal X falls back to the earlier OAM entry
    //        (stable_sort preserves OAM order for equal keys).
    //   CGB: lower OAM index always wins, X is not part of the comparison.
    // The visible[] array was filled in OAM-index order during the scan, so
    // skipping the sort on CGB leaves it in the priority order the loop below
    // wants (highest priority first).
    if (!mmu_.cgb_mode()) {
        std::stable_sort(visible.begin(), visible.begin() + n_visible,
                         [](const sprite_entry& a, const sprite_entry& b) { return a.x < b.x; });
    }

    // Draw in priority order with a per-pixel "claimed" mask so lower-priority
    // sprites cannot poke through a higher-priority sprite's opaque pixels —
    // even when that pixel lost to BG via the BG-priority bit.
    std::array<bool, gb::LCD_WIDTH> claimed{};

    const bool cgb = mmu_.cgb_mode();
    // CGB master-priority bit: when LCDC.0 == 0 the BG/window priority gate
    // is bypassed entirely and sprites always win against opaque BG pixels.
    // The bit has no meaning on DMG here — sprite vs. BG arbitration there
    // is governed solely by the per-sprite bg_priority attribute.
    const bool bg_master_priority = (lcdc & gb::lcdc::bg_enable) != 0;

    for (std::size_t s = 0; s < n_visible; ++s) {
        const auto& sp = visible[s];
        const int sprite_top = static_cast<int>(sp.y) - 16;
        const int sprite_left = static_cast<int>(sp.x) - 8;

        const bool y_flip = (sp.attr & gb::oam_attr::y_flip) != 0;
        const bool x_flip = (sp.attr & gb::oam_attr::x_flip) != 0;
        const bool bg_priority = (sp.attr & gb::oam_attr::bg_priority) != 0;
        // DMG picks between OBP0/OBP1 via bit 4. CGB ignores bit 4 and selects
        // one of the eight OBJ palettes (OCPS-indexed) via bits 0-2 instead.
        const palette_id pal = cgb ? static_cast<palette_id>(static_cast<std::uint8_t>(palette_id::cgb_obj0) +
                                                             (sp.attr & gb::oam_attr::cgb_palette_mask))
                                   : ((sp.attr & gb::oam_attr::dmg_palette_obp1) ? palette_id::obj1 : palette_id::obj0);
        // CGB-only: bit 3 picks which VRAM bank holds the sprite's tile data.
        const std::uint8_t tile_bank = (cgb && (sp.attr & gb::oam_attr::cgb_vram_bank)) ? 1 : 0;

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
        const std::uint16_t data_addr =
            static_cast<std::uint16_t>(gb::TILE_DATA_UNSIGNED_BASE + tile_index * gb::TILE_BYTES + row * 2);
        const std::uint8_t lo = mmu_.vram_read(static_cast<std::uint16_t>(data_addr - gb::VRAM_BASE), tile_bank);
        const std::uint8_t hi = mmu_.vram_read(static_cast<std::uint16_t>(data_addr - gb::VRAM_BASE + 1), tile_bank);

        for (int px = 0; px < gb::TILE_PIXELS; ++px) {
            const int screen_x = sprite_left + px;
            if (screen_x < 0 || screen_x >= gb::LCD_WIDTH)
                continue;
            if (claimed[screen_x])
                continue;

            const std::uint8_t bit = static_cast<std::uint8_t>(x_flip ? px : (7 - px));
            const std::uint8_t ci = tile_color_index(lo, hi, bit);
            if (ci == 0)
                continue; // sprite color 0 = transparent (no claim)

            claimed[screen_x] = true;
            // BG-over-OBJ arbitration:
            //   DMG: sprite goes behind BG when its own bg_priority bit is set
            //        and the BG pixel is non-transparent (color_index != 0).
            //   CGB: the BG wins when LCDC.0 master priority is on AND the BG
            //        pixel is non-transparent AND either the sprite carries
            //        bg_priority OR the BG tile attribute's own priority bit
            //        is set.  With master priority off (LCDC.0=0) sprites
            //        always win regardless of either priority bit.
            const auto& bgp = bg_attr_line[screen_x];
            const bool bg_wins =
                cgb ? (bg_master_priority && bgp.color_index != 0 && (bg_priority || bgp.attr.priority))
                    : (bg_priority && bgp.color_index != 0);
            if (bg_wins)
                continue;

            fb[ly * gb::LCD_WIDTH + screen_x] = resolve_pixel(pal, ci);
        }
    }
}

void ppu::update_lyc_coincidence() {
    const bool coinc = (mmu_.hwr_ly() == mmu_.hwr_lyc());
    mmu_.hwr_stat((mmu_.hwr_stat() & ~gb::stat::lyc_coincidence) | (coinc ? gb::stat::lyc_coincidence : 0));
    if (coinc && (mmu_.hwr_stat() & gb::stat::lyc_irq_enable))
        irq_.request(irq::source::lcd_stat);
}

bool ppu::consume_frame_ready() {
    bool was_ready = frame_ready;
    frame_ready = false;
    return was_ready;
}

void ppu::reset() {
    mode = mode_e::OAM_SCAN;
    dots_in_mode = 0;
    frame_ready = false;
    lcd_was_off = true;
    window_triggered = false;
    window_line = 0;
    fb.fill(0);
    bg_attr_line.fill({0, DMG_BG_ATTR});
}
