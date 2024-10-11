#pragma once

#include <cfg.h>

class Application {
public:
	Application();
	~Application();

	void run();
private:
	gbemu::config cfg;
};
