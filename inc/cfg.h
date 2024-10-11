#pragma once
#include <string>
#include <cstdint>

namespace gbemu {
	struct config {
		explicit config(std::string const& file_path);

		//
		struct romcfg {
			std::string rom_path;
		} rom;

		struct wincfg {
			std::size_t width;
			std::size_t height;
		} win;
	};
}