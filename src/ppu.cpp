#include <ppu.h>

using namespace gbemu;

ppu::ppu(mmu &m) : m(m) {}

void ppu::tick(std::uint32_t t_cycles)
{
    const auto lcdc = m.hwr_lcdc();
    if (!(lcdc & 0x80))
    { // LCD off
        dot_counter = 0;
        m.hwr_ly(0);
        // STAT mode bits a 0 quando off
        m.hwr_stat(m.hwr_stat() & 0xFC);
        mode = 0;
        return;
    }

    dot_counter += t_cycles;
    while (dot_counter >= 456)
    {
        dot_counter -= 456;

        std::uint8_t ly = m.hwr_ly();

        // Render della scanline appena finita (se visibile).
        if (ly < 144)
        {
            render_bg_scanline(ly);
        }

        // Bump LY (con wrap a 154).
        ly = (ly + 1) % 154;
        m.hwr_ly(ly);

        if (ly == 144)
        {
            // Ingresso VBlank
            update_stat_mode(1);
            request_irq(0);          // VBlank → IF.0
            if (m.hwr_stat() & 0x10) // STAT mode-1 enable
                request_irq(1);      // STAT  → IF.1
            frame_ready = true;
        }
        else if (ly < 144)
        {
            update_stat_mode(2); // OAM scan inizio scanline
        }
        // else ly ∈ [145,153]: si resta in VBlank (mode 1)

        update_lyc_coincidence();
    }
}

void ppu::render_bg_scanline(std::uint8_t ly)
{
    static constexpr std::array<std::uint32_t, 4> palette = {
        0xFFFFFFFFu, 0xFFAAAAAAu, 0xFF555555u, 0xFF000000u};

    const auto lcdc = m.hwr_lcdc();
    const auto scy = m.hwr_scy();
    const auto scx = m.hwr_scx();
    const auto bgp = m.hwr_bgp();

    // LCDC.0 = BG enable. If off, fill scanline with shade 0 of BGP.
    if (!(lcdc & 0x01))
    {
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

    for (int px = 0; px < 160; ++px)
    {
        const std::uint8_t x = static_cast<std::uint8_t>(px + scx);
        const std::uint8_t tile_x = x >> 3;
        const std::uint8_t col = 7 - (x & 7);

        const std::uint8_t idx = m.vram[map_base - 0x8000 + tile_y * 32 + tile_x];

        const std::uint16_t data_addr = data_8000
                                            ? static_cast<std::uint16_t>(0x8000 + idx * 16 + row * 2)
                                            : static_cast<std::uint16_t>(0x9000 + static_cast<std::int8_t>(idx) * 16 + row * 2);

        const std::uint8_t lo = m.vram[data_addr - 0x8000];
        const std::uint8_t hi = m.vram[data_addr - 0x8000 + 1];

        const std::uint8_t ci = static_cast<std::uint8_t>((((hi >> col) & 1) << 1) | ((lo >> col) & 1));
        const std::uint8_t shade = (bgp >> (ci * 2)) & 0x03;

        fb[ly * 160 + px] = palette[shade];
    }
}

void ppu::update_stat_mode(std::uint8_t new_mode)
{
    m.hwr_stat((m.hwr_stat() & 0xFC) | (new_mode & 0x03));
    mode = new_mode;
    // STAT sources → IF.1
    if (new_mode == 2 && (m.hwr_stat() & 0x20))
        request_irq(1);
    if (new_mode == 0 && (m.hwr_stat() & 0x08))
        request_irq(1);
    // mode 1: gestito nel chiamante perché coincide con VBlank
}

void ppu::update_lyc_coincidence()
{
    const bool coinc = (m.hwr_ly() == m.hwr_lyc());
    m.hwr_stat((m.hwr_stat() & ~0x04) | (coinc ? 0x04 : 0x00));
    if (coinc && (m.hwr_stat() & 0x40))
        request_irq(1); // STAT → IF.1
}

void ppu::request_irq(std::uint8_t bit)
{
    m.hwr_if(m.hwr_if() | (1 << bit));
}

bool ppu::consume_frame_ready()
{
    bool was_ready = frame_ready;
    frame_ready = false;
    return was_ready;
}