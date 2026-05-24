#pragma once

#include <string>
#include <string_view>

#include <cfg.h>

class Application {
public:
    Application();
    ~Application();

    void set_rom_file(std::string_view path);
    void set_headless(bool h) { headless_ = h; }
    void set_script_path(std::string_view p) { script_path_ = p; }
    void set_output_path(std::string_view p) { output_path_ = p; }

    void run();

private:
    gbemu::config cfg;
    bool headless_{false};
    std::string script_path_{};
    std::string output_path_{};
};
