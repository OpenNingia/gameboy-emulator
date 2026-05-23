#pragma once

#include <cfg.h>
#include <string_view>

class Application {
public:
    Application();
    ~Application();

    void set_rom_file(std::string_view path);
    void run();

private:
    gbemu::config cfg;
};
