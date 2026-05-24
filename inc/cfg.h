#pragma once
#include <string>

namespace gbemu {
    struct config {
        explicit config(std::string const& file_path);

        //
        struct romcfg {
            std::string path;
        } rom;

        struct bioscfg {
            std::string path;
            bool skip;
        } bios;
    };
} // namespace gbemu
