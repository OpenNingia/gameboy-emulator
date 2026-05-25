#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <display_post.h>
#include <log.h>

namespace gbemu::display {

    namespace {

        // Helpers for assembling ARGB pixels from raw (r, g, b) triples.
        // The PPU framebuffer uses ARGB8888 with the alpha byte always
        // 0xFF; reusing that convention here keeps the comparison with
        // gb::DMG_PALETTE_ARGB consistent.
        constexpr std::uint32_t make_argb(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
            return (std::uint32_t{0xFFu} << 24) | (std::uint32_t{r} << 16) | (std::uint32_t{g} << 8) | std::uint32_t{b};
        }

        // ---- Built-in palettes -----------------------------------------
        //
        // RGB values lifted verbatim from SameBoy's GB_PALETTE_* constants
        // (Core/display.c).  SameBoy stores 5 colours per palette, darkest
        // first; the 5th is the off-LCD background.  Our four-shade
        // convention is shade 0 = lightest, so the mapping inverts the
        // first four entries (file colours[3] → shade 0, colours[0] →
        // shade 3) and stashes colours[4] in off_lcd_argb for future use.
        //
        //   GREY: classic four-step grey (also matches gb::DMG_PALETTE_ARGB)
        //   DMG : iconic green DMG
        //   MGB : cooler greens of the Game Boy Pocket
        //   GBL : brighter teal-green of the backlit Game Boy Light

        palette make_palette(const char* name, std::uint8_t r0, std::uint8_t g0, std::uint8_t b0, std::uint8_t r1,
                             std::uint8_t g1, std::uint8_t b1, std::uint8_t r2, std::uint8_t g2, std::uint8_t b2,
                             std::uint8_t r3, std::uint8_t g3, std::uint8_t b3, std::uint8_t r4, std::uint8_t g4,
                             std::uint8_t b4) {
            // Input order matches SameBoy: (darkest .. lightest, off_lcd).
            return palette{
                std::string{name},
                {
                    make_argb(r3, g3, b3),                                               // shade 0 = lightest visible
                    make_argb(r2, g2, b2), make_argb(r1, g1, b1), make_argb(r0, g0, b0), // shade 3 = darkest
                },
                make_argb(r4, g4, b4),
            };
        }

        palette builtin_grey() {
            return make_palette("grey", 0x00, 0x00, 0x00, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF);
        }

        palette builtin_dmg() {
            return make_palette("dmg", 0x08, 0x18, 0x10, 0x39, 0x61, 0x39, 0x84, 0xA5, 0x63, 0xC6, 0xDE, 0x8C, 0xD2,
                                0xE6, 0xA6);
        }

        palette builtin_mgb() {
            return make_palette("mgb", 0x07, 0x10, 0x0E, 0x3A, 0x4C, 0x3A, 0x81, 0x8D, 0x66, 0xC2, 0xCE, 0x93, 0xCF,
                                0xDA, 0xAC);
        }

        palette builtin_gbl() {
            return make_palette("gbl", 0x0A, 0x1C, 0x15, 0x35, 0x78, 0x62, 0x56, 0xB4, 0x95, 0x7F, 0xE2, 0xC3, 0x91,
                                0xEA, 0xD0);
        }

        // Mix five ARGB pixels with five normalised weights (sum == 1.0).
        // Pixel components are kept as integers — the cumulative weight
        // is bias-free at 8 bits and ~30× faster than per-pixel float
        // multiplication, which we need for the 23040-pixel framebuffer.
        // `w` is in fixed-point Q16 (i.e. weight * 65536) and must total
        // 65536 across the 5 entries.
        constexpr std::uint32_t mix5(std::uint32_t p0, std::uint32_t p1, std::uint32_t p2, std::uint32_t p3,
                                     std::uint32_t p4, std::array<std::uint32_t, 5> const& w) {
            // Each colour channel: sum(p_i.channel * w_i) >> 16. Alpha is
            // forced to 0xFF — the framebuffer is opaque by contract and
            // mixing 0xFF with 0xFF anyway lands back on 0xFF.
            const std::uint32_t r =
                (((p0 >> 16) & 0xFF) * w[0] + ((p1 >> 16) & 0xFF) * w[1] + ((p2 >> 16) & 0xFF) * w[2] +
                 ((p3 >> 16) & 0xFF) * w[3] + ((p4 >> 16) & 0xFF) * w[4]) >>
                16;
            const std::uint32_t g = (((p0 >> 8) & 0xFF) * w[0] + ((p1 >> 8) & 0xFF) * w[1] + ((p2 >> 8) & 0xFF) * w[2] +
                                     ((p3 >> 8) & 0xFF) * w[3] + ((p4 >> 8) & 0xFF) * w[4]) >>
                                    16;
            const std::uint32_t b = ((p0 & 0xFF) * w[0] + (p1 & 0xFF) * w[1] + (p2 & 0xFF) * w[2] + (p3 & 0xFF) * w[3] +
                                     (p4 & 0xFF) * w[4]) >>
                                    16;
            return (std::uint32_t{0xFFu} << 24) | (r << 16) | (g << 8) | b;
        }

        // Q16 weights for `blend_mode::accurate`. {0.45, 0.27, 0.15, 0.08, 0.05}
        // * 65536, rounded so the integer sum is exactly 65536.
        constexpr std::array<std::uint32_t, 5> ACCURATE_W_Q16 = {29491, 17695, 9830, 5243, 3277};
        // Q16 weights for `blend_mode::simple` shoved into the 5-tap mixer:
        // current + prev-1, with the trailing 3 taps zeroed.
        constexpr std::array<std::uint32_t, 5> SIMPLE_W_Q16 = {32768, 32768, 0, 0, 0};

    } // namespace

    blend_mode parse_blend_mode(std::string_view name) {
        if (name == "accurate")
            return blend_mode::accurate;
        if (name == "simple")
            return blend_mode::simple;
        return blend_mode::disabled;
    }

    // --- palette_registry -------------------------------------------------

    void palette_registry::install_builtins() {
        // Idempotent: if "grey" is already present we assume the rest are
        // too (no partial install path) and bail out.  Callers that want
        // to refresh built-ins must clear the registry first.
        if (find("grey"))
            return;
        entries_.push_back(builtin_grey());
        entries_.push_back(builtin_dmg());
        entries_.push_back(builtin_mgb());
        entries_.push_back(builtin_gbl());
    }

    void palette_registry::scan_directory(const std::string& dir) {
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec)) {
            // No palettes dir → nothing to scan.  Not an error; user may
            // genuinely have no custom palettes installed.
            return;
        }

        for (auto it = std::filesystem::directory_iterator(dir, ec); !ec && it != std::filesystem::directory_iterator();
             it.increment(ec)) {
            const auto& entry = *it;
            if (!entry.is_regular_file(ec))
                continue;
            const auto ext = entry.path().extension().string();
            if (ext != ".sbp" && ext != ".SBP")
                continue;

            palette p;
            // Forward-slash form everywhere — repo convention (CLAUDE.md
            // and the paths memory). lexically_normal collapses any . / ..
            // components before .generic_string() flips the separators.
            const auto path_str = entry.path().lexically_normal().generic_string();
            if (!load_sbp_file(path_str, p)) {
                LOG_WARNING(gbemu::log::root(), "palette: skipping {}", path_str);
                continue;
            }
            // Filename stem becomes the registry key.  Collision with a
            // built-in (or another user file) is resolved last-wins —
            // arguably surprising, but cheap and lets the user override
            // "dmg" with their own .sbp by naming it dmg.sbp.
            p.name = entry.path().stem().string();
            // Drop any conflicting entry first so `find` returns the new one.
            entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                          [&](const palette& existing) { return existing.name == p.name; }),
                           entries_.end());
            entries_.push_back(std::move(p));
        }
    }

    const palette* palette_registry::find(std::string_view name) const {
        for (const auto& p : entries_) {
            if (p.name == name)
                return &p;
        }
        return nullptr;
    }

    // --- .sbp parser ------------------------------------------------------

    bool load_sbp_file(const std::string& path, palette& out) {
        std::ifstream f(path, std::ios::binary);
        if (!f) {
            LOG_WARNING(gbemu::log::root(), "palette: cannot open {}", path);
            return false;
        }
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

        // Minimum file: 4 magic + 1 flags + 12 colour bytes = 17.
        if (bytes.size() < 17) {
            LOG_WARNING(gbemu::log::root(), "palette: {} is too small ({} bytes)", path, bytes.size());
            return false;
        }

        // Magic is the ASCII string "LPBS" written byte-for-byte on disk
        // (it's the little-endian representation of the four-character
        // constant 'SBPL' that SameBoy stores via `uint32_t magic`).
        if (bytes[0] != 'L' || bytes[1] != 'P' || bytes[2] != 'B' || bytes[3] != 'S') {
            LOG_WARNING(gbemu::log::root(), "palette: {} has wrong magic", path);
            return false;
        }

        const std::uint8_t flags = bytes[4];
        const bool has_off_lcd = bytes.size() == 20u;
        /* if (flags != 0x01 && flags != 0x02 && flags != 0x03) {
            LOG_WARNING(gbemu::log::root(), "palette: {} has unknown flags 0x{:02X}", path, flags);
            return false;
        }*/

        /* const std::size_t expected = has_off_lcd ? 20u : 17u;
        if (bytes.size() != expected) {
            LOG_WARNING(gbemu::log::root(), "palette: {} has wrong size ({} bytes, expected {})", path, bytes.size(),
                        expected);
            return false;
        }*/

        // File order is darkest-to-lightest; our shades are lightest-first.
        // Read four RGB triples starting at offset 5 and invert the index.
        std::array<std::uint32_t, 4> shades{};
        for (int i = 0; i < 4; ++i) {
            const std::size_t off = 5u + static_cast<std::size_t>(i) * 3u;
            const std::uint8_t r = bytes[off + 0];
            const std::uint8_t g = bytes[off + 1];
            const std::uint8_t b = bytes[off + 2];
            shades[3 - static_cast<std::size_t>(i)] = make_argb(r, g, b);
        }

        std::uint32_t off_lcd = 0;
        if (has_off_lcd) {
            const std::uint8_t r = bytes[17];
            const std::uint8_t g = bytes[18];
            const std::uint8_t b = bytes[19];
            off_lcd = make_argb(r, g, b);
        }

        out.shades = shades;
        out.off_lcd_argb = off_lcd;
        return true;
    }

    // --- frame_blender ----------------------------------------------------

    frame_blender::frame_blender() = default;

    void frame_blender::set_mode(blend_mode m) {
        mode_ = m;
    }

    void frame_blender::reset() {
        for (auto& slot : history_)
            slot.fill(0);
        output_.fill(0);
        writer_ = 0;
        history_seeded_ = false;
    }

    const std::uint32_t* frame_blender::blend(const std::uint32_t* current) {
        // First frame after a reset: seed *every* history slot with the
        // current frame so the blender's next call doesn't dim the picture
        // by averaging against all-zero slots.  Treats the post-reset
        // moment as "the LCD has been showing this image steadily" — the
        // closest physical analogue to a real LCD whose photons just got
        // their first refresh.
        if (!history_seeded_) {
            for (auto& slot : history_)
                std::memcpy(slot.data(), current, FB_PIXELS * sizeof(std::uint32_t));
            writer_ = 0;
            history_seeded_ = true;
            return current;
        }

        if (mode_ == blend_mode::disabled) {
            // Even in disabled mode we keep filling the ring so that a
            // later switch to simple/accurate has fresh data to blend with.
            std::memcpy(history_[writer_].data(), current, FB_PIXELS * sizeof(std::uint32_t));
            writer_ = (writer_ + 1u) % HISTORY_SLOTS;
            return current;
        }

        // history_[(writer_ - 1) % N] is the most recent previously-pushed
        // frame, [(writer_ - 2) % N] the one before that, etc.  Compute
        // the read indices once outside the pixel loop.
        const std::size_t idx_p1 = (writer_ + HISTORY_SLOTS - 1u) % HISTORY_SLOTS;
        const std::size_t idx_p2 = (writer_ + HISTORY_SLOTS - 2u) % HISTORY_SLOTS;
        const std::size_t idx_p3 = (writer_ + HISTORY_SLOTS - 3u) % HISTORY_SLOTS;
        const std::size_t idx_p4 = (writer_ + HISTORY_SLOTS - 4u) % HISTORY_SLOTS;

        const auto& weights = (mode_ == blend_mode::accurate) ? ACCURATE_W_Q16 : SIMPLE_W_Q16;

        const std::uint32_t* p1 = history_[idx_p1].data();
        const std::uint32_t* p2 = history_[idx_p2].data();
        const std::uint32_t* p3 = history_[idx_p3].data();
        const std::uint32_t* p4 = history_[idx_p4].data();

        for (std::size_t i = 0; i < FB_PIXELS; ++i) {
            output_[i] = mix5(current[i], p1[i], p2[i], p3[i], p4[i], weights);
        }

        // Push the *current* frame (not the blended one) into history.
        // Mixing blended output back in would low-pass the signal across
        // generations and over-smooth the picture far more than the real
        // DMG does.
        std::memcpy(history_[writer_].data(), current, FB_PIXELS * sizeof(std::uint32_t));
        writer_ = (writer_ + 1u) % HISTORY_SLOTS;

        return output_.data();
    }

} // namespace gbemu::display
