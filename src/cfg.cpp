#include <cfg.h>
#include <libconfig.h++>

using namespace gbemu;
using namespace libconfig;

config::config(std::string const& file_path)
{
	Config cfg;
	cfg.readFile(file_path);

	const auto& root = cfg.getRoot();
	const auto& app_rom = root["application"]["rom"];

	app_rom.lookupValue("path", rom.rom_path);

	auto& valw = root.lookup("application.window.size.w");
	auto& valh = root.lookup("application.window.size.h");

	win.width = valw.isNumber() ? static_cast<int>(valw) : 640;
	win.height = valh.isNumber() ? static_cast<int>(valh) : 480;
}