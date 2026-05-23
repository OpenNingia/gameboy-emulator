#include <log.h>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/sinks/FileSink.h>

namespace gbemu::log {

    static quill::Logger* g_root = nullptr;

    void init() {
        if (g_root)
            return;

        quill::Backend::start();

        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w'); // truncate ad ogni avvio
        cfg.set_filename_append_option(quill::FilenameAppendOption::None);

        auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>("gbemu.log", cfg);

        g_root = quill::Frontend::create_or_get_logger("root", std::move(sink));
        g_root->set_log_level(quill::LogLevel::Debug);
    }

    quill::Logger* root() {
        return g_root;
    }

} // namespace gbemu::log
