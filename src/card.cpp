#include <card.h>
#include <exc.hpp>

#include <filesystem>
#include <fstream>

using namespace gbemu;

void cartridge::load_rom(const std::string& path)
{
	std::filesystem::path input_file_path{ path };
	auto length = std::filesystem::file_size(path);

	std::ifstream fs/*{path, std::ios::binary}*/;
	fs.open(path, std::ios::binary);
	if (!fs.good())
		throw gbemu_exception{ "Could not load rom!" };

	// start from beginning
	fs.seekg(0);

	// reserve length
	data.resize(length);

	fs.read(reinterpret_cast<char*>(data.data()), length);
	fs.close();
}