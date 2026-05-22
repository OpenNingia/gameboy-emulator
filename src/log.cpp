#include <log.h>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/sinks/ConsoleSink.h>

namespace gbemu::log {

	static quill::Logger* g_root = nullptr;

	void init() {
		if (g_root) return;

		quill::Backend::start();

		g_root = quill::Frontend::create_or_get_logger(
			"root",
			quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console"));

		g_root->set_log_level(quill::LogLevel::Debug);
	}

	quill::Logger* root() {
		return g_root;
	}

}
