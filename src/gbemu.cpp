#include <app.h>
#include <log.h>

int main(int argc, char* argv[]) {
    gbemu::log::init();
    try {
        Application app;

        if (argc > 1) {
            app.set_rom_file(argv[1]);
        }

        app.run();
    } catch (const std::exception& e) {
        LOG_ERROR(gbemu::log::root(), "Error: {}", e.what());
        gbemu::log::root()->flush_log();
        return 1;
    }
    return 0;
}
