#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace gbemu::paths {

    // Resolve the on-disk root used to anchor cfg/bios/roms/savs/sslots.
    // Order:
    //   1) $GBEMU_HOME if set to a non-empty value (caller wins; useful for
    //      CI, sandboxed tests, and split-config setups);
    //   2) the directory containing the running executable, via
    //      SDL_GetBasePath();
    //   3) "." as a last-resort fallback if SDL refuses to answer.
    // Always returned without a trailing separator so callers can join with
    // a single "/" / "\\". Computed once per process — cache the result.
    std::string resolve_base_dir();

    // Join `sub` under `base`. If `sub` is absolute (filesystem::path::
    // is_absolute), it is returned unchanged; otherwise the result is
    // `base / sub` using the platform's preferred separator.
    std::string resolve_under(std::string_view base, std::string_view sub);

    // Resolve a user-supplied data path (e.g. positional ROM argument
    // or cfg.bios.path) against `<base>/<subdir>`:
    //   - absolute path  -> returned unchanged (escape hatch for files
    //                       living outside the portable layout)
    //   - anything else  -> joined under <base>/<subdir>, separators in
    //                       the relative part preserved (so
    //                       `ita/foo.gb` resolves to
    //                       `<base>/<subdir>/ita/foo.gb`)
    std::string resolve_data_path(std::string_view base, std::string_view subdir, std::string_view name);

    // Deterministic 64-bit FNV-1a hash over a byte span, formatted as a
    // 16-char lowercase hex string. Used to derive stable filenames for
    // battery saves and save-state slots from a cartridge image so the
    // user can rename the ROM file without orphaning the save.
    std::string fnv1a_64_hex(std::span<const std::uint8_t> bytes);

} // namespace gbemu::paths
