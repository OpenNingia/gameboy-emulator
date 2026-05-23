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
    render_bg_scanline(m.hwr_ly());
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

    // LCDC.0 = BG enable. If off, fill scanline with shade 0 of BGP.
    if (!(lcdc & 0x01)) {
        const auto bg_off = palette[bgp & 0x03];
        for (int px = 0; px < 160; ++px)
            fb[ly * 160 + px] = bg_off;
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
