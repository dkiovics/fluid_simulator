#pragma once
#include <GLFW/glfw3.h>
#include <mutex>
#include <memory>
#include <atomic>
#include "engine/windowManager.h"
#include "manager/simulationManager.h"
#include "gfxInterface/gfxInterface.hpp"

namespace controls
{

constexpr int initialScreenWidth = 1800;
constexpr int initialScreenHeight = 1200;
const glm::dvec3 dimensions2D(40, 22.5, 3);
const glm::dvec3 dimensions3D(40, 25, 20);

class ControlWindow
{
private:
	ControlWindow();
	static std::unique_ptr<ControlWindow> instance;

	GLFWwindow* window;
	std::shared_ptr<renderer::WindowManager> windowManager;

	genericfsim::manager::SimulationConfig simConfig2D;
	genericfsim::manager::SimulationConfig simConfig3D;

	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager;
	std::unique_ptr<visual::GfxInterface> simulatorRenderer;

	std::atomic<bool> running;
	std::mutex guiLock;

	void startThreadWorker();
	void threadWorker();

public:
	void start();

};


}
