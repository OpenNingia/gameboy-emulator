#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace gbemu::bess {

    // SameBoy BESS (Best Effort Save State) — partial implementation
    // covering what battery saves need: a SRAM-prefixed buffer with a
    // BESS footer carrying NAME + RTC + END blocks.
    //
    // Format reference: https://github.com/LIJI32/SameBoy/blob/master/BESS.md
    //
    // The on-disk layout is:
    //
    //   [ SRAM raw bytes ]                          // prefix, interop-compatible
    //   [ NAME block ]   id="NAME", payload = ASCII emulator name + version
    //   [ RTC  block ]   id="RTC ", payload = 48 bytes (only on MBC3+TIMER)
    //   [ END  block ]   id="END ", size = 0
    //   [ u32 LE: offset to the first block ]
    //   [ "BESS" 4 ASCII bytes ]                    // last 4 bytes of file
    //
    // The footer probe never touches the SRAM prefix, so a foreign
    // emulator that doesn't understand BESS reads our file as a pure
    // SRAM dump and ignores the trailing metadata.  We do the same for
    // foreign .sav files: if the footer probe fails (wrong magic, bad
    // offset, malformed block list) we fall back to treating the whole
    // file as raw SRAM.

    struct parse_result {
        // Decoded SRAM prefix.  Always populated — never empty in a
        // well-formed file, but a caller-side size check is still
        // required (the MBC API rejects mismatched loads).
        std::vector<std::uint8_t> sram;
        // RTC block payload, 48 bytes in the BESS layout.  nullopt when
        // the file has no BESS footer or the footer carries no 'RTC '
        // block (e.g. battery save from a non-RTC cart).
        std::optional<std::array<std::uint8_t, 48>> rtc;
    };

    // Parse a .sav buffer.  Never throws — corrupted footers are
    // silently downgraded to "all bytes are SRAM, no RTC".  Returns
    // empty sram if the buffer is empty.
    parse_result parse_sav(std::span<const std::uint8_t> buf);

    // Serialize a .sav buffer.  `sram` is written verbatim at the
    // start; the BESS footer follows with a NAME block carrying
    // `emulator_name`, an optional RTC block (only when `rtc` is set),
    // an END block, and the 8-byte footer.  When `sram` is empty and
    // `rtc` is nullopt the result is an empty vector (no file to
    // write).
    std::vector<std::uint8_t> write_sav(std::span<const std::uint8_t> sram,
                                        std::optional<std::array<std::uint8_t, 48>> rtc,
                                        std::string_view emulator_name);

} // namespace gbemu::bess
