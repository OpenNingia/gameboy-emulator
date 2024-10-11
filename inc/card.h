#pragma once

#include <vector>
#include <string>

namespace gbemu {
	struct cartridge {
		void load_rom(const std::string& path);
		std::vector<std::uint8_t> data{};
	};
}