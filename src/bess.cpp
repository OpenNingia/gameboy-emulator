#include <cstring>

#include <bess.h>

namespace gbemu::bess {

    namespace {

        constexpr std::size_t footer_size = 8;       // u32 LE offset + "BESS"
        constexpr std::size_t block_header_size = 8; // 4-byte ID + u32 LE size

        std::uint32_t read_u32_le(const std::uint8_t* p) {
            return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
                   (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
        }

        void append_u32_le(std::vector<std::uint8_t>& out, std::uint32_t v) {
            out.push_back(static_cast<std::uint8_t>(v & 0xFF));
            out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
            out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
            out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        }

        void append_block(std::vector<std::uint8_t>& out, const char (&id)[5], std::span<const std::uint8_t> payload) {
            // id is the 5-char string literal — keep the first 4 chars
            // verbatim, which preserves the trailing space in "RTC " and
            // "END ".  No NUL-termination is written.
            out.insert(out.end(), id, id + 4);
            append_u32_le(out, static_cast<std::uint32_t>(payload.size()));
            out.insert(out.end(), payload.begin(), payload.end());
        }

    } // namespace

    parse_result parse_sav(std::span<const std::uint8_t> buf) {
        parse_result r;
        if (buf.empty())
            return r;

        // First pass: is there a valid BESS footer? Need at least 8 bytes
        // and the magic "BESS" in the last 4.  The offset must point at
        // something that could be a block header inside the buffer.
        bool has_footer = false;
        std::uint32_t first_block_off = 0;
        if (buf.size() >= footer_size) {
            const std::uint8_t* footer = buf.data() + buf.size() - footer_size;
            if (footer[4] == 'B' && footer[5] == 'E' && footer[6] == 'S' && footer[7] == 'S') {
                first_block_off = read_u32_le(footer);
                // The offset must leave room for at least one block + the
                // 8-byte footer itself.  Reject anything bogus.
                if (first_block_off + block_header_size <= buf.size() - footer_size)
                    has_footer = true;
            }
        }

        if (!has_footer) {
            // Treat the entire buffer as SRAM — supports old / foreign
            // emulator .sav files transparently.
            r.sram.assign(buf.begin(), buf.end());
            return r;
        }

        // SRAM prefix is everything before the first block.  The BESS
        // spec doesn't require SRAM to live at offset 0, but every
        // emulator that emits a battery save with BESS does — and so do
        // we.  A non-zero gap between SRAM end and the first block would
        // mean undefined bytes, which we won't try to recover.
        r.sram.assign(buf.begin(), buf.begin() + first_block_off);

        // Walk the block list until END (or buffer exhaustion).  Bail
        // out gracefully on anything malformed; the SRAM prefix has
        // already been captured.
        std::size_t off = first_block_off;
        const std::size_t end_off = buf.size() - footer_size;
        while (off + block_header_size <= end_off) {
            const std::uint8_t* hdr = buf.data() + off;
            const std::uint32_t size = read_u32_le(hdr + 4);
            const std::size_t payload_off = off + block_header_size;
            if (payload_off + size > end_off)
                break; // truncated block
            if (hdr[0] == 'E' && hdr[1] == 'N' && hdr[2] == 'D' && hdr[3] == ' ')
                break; // end of block list — last legal block
            if (hdr[0] == 'R' && hdr[1] == 'T' && hdr[2] == 'C' && hdr[3] == ' ' && size == 48) {
                std::array<std::uint8_t, 48> blob{};
                std::memcpy(blob.data(), buf.data() + payload_off, 48);
                r.rtc = blob;
            }
            // Other blocks (NAME, INFO, CORE, MBC, ...) are ignored —
            // they carry save-state metadata we don't consume in the
            // battery-save path.
            off = payload_off + size;
        }
        return r;
    }

    std::vector<std::uint8_t> write_sav(std::span<const std::uint8_t> sram,
                                        std::optional<std::array<std::uint8_t, 48>> rtc,
                                        std::string_view emulator_name) {
        if (sram.empty() && !rtc.has_value())
            return {};

        std::vector<std::uint8_t> out;
        out.reserve(sram.size() + 128);
        out.insert(out.end(), sram.begin(), sram.end());

        const std::uint32_t first_block_off = static_cast<std::uint32_t>(out.size());

        // NAME — emulator identification, conventionally first.
        append_block(out, "NAME",
                     std::span<const std::uint8_t>{reinterpret_cast<const std::uint8_t*>(emulator_name.data()),
                                                   emulator_name.size()});

        // RTC  — only when the caller has a payload.
        if (rtc.has_value())
            append_block(out, "RTC ", std::span<const std::uint8_t>{rtc->data(), rtc->size()});

        // END  — empty payload, marks end of block list.
        append_block(out, "END ", std::span<const std::uint8_t>{});

        // Footer: u32 LE offset to first block + "BESS".
        append_u32_le(out, first_block_off);
        out.push_back('B');
        out.push_back('E');
        out.push_back('S');
        out.push_back('S');
        return out;
    }

} // namespace gbemu::bess
