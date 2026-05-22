#include <app.h>
#include <log.h>

int main()
{
    gbemu::log::init();
    try {
        Application app;
        app.run();
    } catch (const std::exception& e) {
        LOG_ERROR(gbemu::log::root(), "Error: {}", e.what());
        gbemu::log::root()->flush_log();
        return 1;
    }
    return 0;
}
