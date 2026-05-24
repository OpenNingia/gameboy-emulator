#include <array>
#include <cstdlib>
#include <filesystem>

#include <SDL2/SDL.h>
#include <paths.h>

namespace gbemu::paths {

    namespace {
        // Normalize a freshly-constructed std::filesystem::path back to a
        // generic (forward-slash) string. Forward slashes work everywhere
        // — Windows accepts them in fopen / std::filesystem APIs — and
        // sidestep the escape-doubling headache when paths flow through
        // shells, log lines, debugger watch windows, and string parsers.
        std::string to_generic(std::filesystem::path const& p) {
            std::string s = p.lexically_normal().generic_string();
            // Strip a trailing slash (preserves single leading `//` UNC
            // roots, which we leave alone).
            while (s.size() > 1 && s.back() == '/')
                s.pop_back();
            return s;
        }
    } // namespace

    std::string resolve_base_dir() {
        // $GBEMU_HOME wins when set — escape hatch for sandboxed tests,
        // CI runs, and users who keep config separate from the install.
        if (const char* env = std::getenv("GBEMU_HOME"); env && *env) {
            return to_generic(std::filesystem::path{env});
        }

        // SDL_GetBasePath returns the directory of the executable with a
        // trailing platform separator. The to_generic helper strips it
        // and folds any native separators back to `/`.
        if (char* base = SDL_GetBasePath()) {
            std::string s{base};
            SDL_free(base);
            if (!s.empty()) {
                return to_generic(std::filesystem::path{s});
            }
        }

        return ".";
    }

    std::string resolve_under(std::string_view base, std::string_view sub) {
        std::filesystem::path p{sub};
        if (p.is_absolute())
            return to_generic(p);
        return to_generic(std::filesystem::path{base} / p);
    }

    std::string resolve_data_path(std::string_view base, std::string_view subdir, std::string_view name) {
        std::filesystem::path p{name};
        // Absolute path -> user gave us an explicit location, honor it.
        // Everything else (bare names AND relative paths with separators,
        // e.g. "ita/foo.gb") joins under <base>/<subdir>. The user's
        // mental model: subdirectories under roms/ are normal, the escape
        // hatch for "ROM living outside the layout" is an absolute path.
        if (p.is_absolute())
            return to_generic(p);
        return resolve_under(resolve_under(base, subdir), name);
    }

    std::string fnv1a_64_hex(std::span<const std::uint8_t> bytes) {
        // Standard 64-bit FNV-1a parameters.
        constexpr std::uint64_t offset_basis = 0xCBF29CE484222325ULL;
        constexpr std::uint64_t prime = 0x100000001B3ULL;

        std::uint64_t h = offset_basis;
        for (auto b : bytes) {
            h ^= static_cast<std::uint64_t>(b);
            h *= prime;
        }

        // Format as 16-char zero-padded lowercase hex, MSB first.
        std::array<char, 17> buf{};
        constexpr char digits[] = "0123456789abcdef";
        for (int i = 15; i >= 0; --i) {
            buf[i] = digits[h & 0xF];
            h >>= 4;
        }
        buf[16] = '\0';
        return std::string{buf.data(), 16};
    }

} // namespace gbemu::paths
