#include <spdlog/spdlog.h>
#include <memory>
#include <chrono>

#include "controlRegistry.h"
#include "controlUi.h"
#include "manager/simulationManager.h"
#include "gfxInterface.hpp"
#include "2D/visuals2D.h"
#include "3D/visuals3D.h"


using namespace genericfsim;


static constexpr int initialWidth = 1600;
static constexpr int initialHeight = 900;
static constexpr float aspectRatio = (float)initialWidth / (float)initialHeight;

static const glm::dvec3 dimensions2D(40, 40 / aspectRatio, 3);
static const glm::dvec3 dimensions3D(40, 25, 20);

static std::shared_ptr<manager::SimulationManager> simulationManager = nullptr;
static std::shared_ptr<visual::GfxInterface> graphicsInterface = nullptr;


static bool handleSimManagerChange(bool is2D)
{
	if (!simulationManager || simulationManager->twoD && !is2D
		|| !simulationManager->twoD && is2D)
	{
		if (is2D)
			simulationManager = std::make_shared<manager::SimulationManager>(dimensions2D, true);
		else
			simulationManager = std::make_shared<manager::SimulationManager>(dimensions3D, false);
		return true;
	}
	return false;
}

static visual::SceneConfig getSceneConfig()
{
	visual::SceneConfig sceneConfig { };
	sceneConfig.simSize = simulationManager->getDimensions();
	sceneConfig.simCenter = sceneConfig.simSize * 0.5f;
	sceneConfig.gridResolution = simulationManager->getGridSize();
	return sceneConfig;
}

static bool handleGraphicsInterfaceChange(renderer::WindowManager& window, bool is2D)
{
	if (!graphicsInterface || dynamic_cast<visual::Visuals2D*>(graphicsInterface.get()) && !is2D
		|| dynamic_cast<visual::Visuals3D*>(graphicsInterface.get()) && is2D)
	{
		if (is2D)
			graphicsInterface = std::make_shared<visual::Visuals2D>(window, getSceneConfig());
		else
			graphicsInterface = std::make_shared<visual::Visuals3D>(window, getSceneConfig());
		return true;
	}
	return false;
}


void runApplication()
{
	auto& registry = controls::ControlRegistry::getInstance();

	renderer::WindowManager simWindow(initialWidth, initialHeight, "Simulation");
	controls::ControlWindow controlWindow(450, 1);
	controlWindow.start();

	simWindow.makeWindowContextcurrent();
	auto windowHandle = simWindow.getWindow();
	simWindow.makeWindowContextcurrent();

	double prevDt = 0.0;

	while (!glfwWindowShouldClose(windowHandle) && controlWindow.isRunning())
	{
		double dt = simWindow.getLastFrameTime();
		prevDt = 0.9 * prevDt + 0.1 * dt;
		registry["telemetry.fps"].set(float(1.0 / prevDt));

		glfwPollEvents();

		if (registry["app.is_2d"])
			registry["app.is_2d_sim"] = true;

		handleSimManagerChange(registry["app.is_2d_sim"]);

		handleGraphicsInterfaceChange(simWindow, registry["app.is_2d"]);

		simulationManager->stepSimulation(dt);

		graphicsInterface->setSceneConfig(getSceneConfig());
		graphicsInterface->handleUserInteractions();
		graphicsInterface->render(simulationManager->getParticleData());

		simWindow.swapBuffers();
	}

	controlWindow.stop();

	graphicsInterface.reset();
	simulationManager.reset();
}
