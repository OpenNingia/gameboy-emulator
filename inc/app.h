#pragma once

#include <string_view>

#include <cfg.h>

class Application {
public:
    Application();
    ~Application();

    void set_rom_file(std::string_view path);
    void run();

private:
    gbemu::config cfg;
};
