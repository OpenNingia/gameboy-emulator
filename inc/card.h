#pragma once

#include <vector>
#include <string>

namespace gbemu {
	struct bin_file {
		void load_from(const std::string& path);
		std::vector<std::uint8_t> data{};
	};

	struct rom_file : bin_file {};
	struct bios_file : bin_file {};
}