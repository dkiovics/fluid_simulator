#pragma once
#include <memory>
#include "engine/windowManager.h"


namespace controls
{

class _ControlCollection;

class ControlWindow
{
private:
	std::unique_ptr<_ControlCollection> controlCollection;

public:
	ControlWindow();

	ControlWindow(const ControlWindow&) = delete;
	ControlWindow& operator=(const ControlWindow&) = delete;
	ControlWindow(ControlWindow&&) = delete;
	ControlWindow& operator=(ControlWindow&&) = delete;

	~ControlWindow();

	void init(renderer::WindowManager& window);
	void render();
	void shutdown();
};

}
