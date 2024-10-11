#pragma once

#include <stdexcept>

namespace gbemu {
	struct gbemu_exception : std::runtime_error {
		gbemu_exception(std::string const& what): std::runtime_error(what) { }
	};
}