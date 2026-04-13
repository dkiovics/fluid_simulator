#include <spdlog/spdlog.h>
#include <chrono>
#include <memory>

#include "2D/visuals2D.h"
#include "3D/visuals3D.h"
#include "controlRegistry.h"
#include "controlUi.h"
#include "gfxInterface.hpp"
#include "manager/simulationManager.h"
#include "prefixTest.h"
#include "stateReconstructionController.h"

using namespace genericfsim;

static constexpr int initialWidth = 1600;
static constexpr int initialHeight = 900;
static constexpr float aspectRatio = (float) initialWidth / (float) initialHeight;

static const glm::dvec3 dimensions2D(40, 40 / aspectRatio, 3);
static const glm::dvec3 dimensions3D(40, 25, 20);

static std::shared_ptr<manager::SimulationManager> simulationManager = nullptr;
static std::shared_ptr<visual::GfxInterface> graphicsInterface = nullptr;
static std::unique_ptr<diffrender::StateReconstructionController> stateReconstructionController = nullptr;

static bool handleSimManagerChange(bool is2D)
{
    if (!simulationManager || simulationManager->twoD && !is2D || !simulationManager->twoD && is2D)
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
    visual::SceneConfig sceneConfig{};
    sceneConfig.simSize = simulationManager->getDimensions();
    sceneConfig.simCenter = sceneConfig.simSize * 0.5f;
    sceneConfig.gridResolution = simulationManager->getGridSize();
    return sceneConfig;
}

static void updateSimulationObstacles()
{
    auto& obstacles = graphicsInterface->getObstacles();
    std::vector<std::unique_ptr<genericfsim::obstacle::Obstacle>> simObstacles;
    for (auto& o : obstacles)
    {
        if (o.type == visual::Obstacle::ObstacleType::BOX)
        {
            simObstacles.push_back(
                std::make_unique<genericfsim::obstacle::RectengularObstacle>(o.size, o.position, o.prevPosition));
        }
        else if (o.type == visual::Obstacle::ObstacleType::SPHERE)
        {
            simObstacles.push_back(std::make_unique<genericfsim::obstacle::SphericalObstacle>(
                o.size.x * 0.5f, o.position, o.prevPosition));
        }
    }
    simulationManager->setObstacles(std::move(simObstacles));
}

static bool handleGraphicsInterfaceChange(renderer::WindowManager& window, bool is2D)
{
    if (!graphicsInterface || dynamic_cast<visual::Visuals2D*>(graphicsInterface.get()) && !is2D ||
        dynamic_cast<visual::Visuals3D*>(graphicsInterface.get()) && is2D)
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
    /*runPrefixTest();
    return;*/

    auto& registry = controls::ControlRegistry::getInstance();

    renderer::WindowManager simWindow(initialWidth, initialHeight, "Simulation");
    controls::ControlWindow controlWindow(450, 1);
    controlWindow.start();

    simWindow.makeWindowContextcurrent();
    glfwSwapInterval(0);
    auto windowHandle = simWindow.getWindow();

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

        if (registry["app.state_reconstruction_enabled"])
        {
            bool paramSettingsValid = !registry["app.is_2d"] && !registry["app.is_2d_sim"];
            if (!paramSettingsValid)
            {
                spdlog::warn(
                    "State reconstruction is only supported for 3D simulations, disabling state reconstruction");
                registry["app.state_reconstruction_enabled"] = false;
            }
            else if (!stateReconstructionController)
            {
                stateReconstructionController = std::make_unique<diffrender::StateReconstructionController>(
                    simWindow, simulationManager, std::dynamic_pointer_cast<visual::Visuals3D>(graphicsInterface));
            }
        }
        else
        {
            stateReconstructionController = nullptr;
        }

        graphicsInterface->setSceneConfig(getSceneConfig());
        graphicsInterface->handleUserInteractions();
        updateSimulationObstacles();

        simulationManager->stepSimulation(dt);

        if (stateReconstructionController)
            stateReconstructionController->processAndRender();
        else
            graphicsInterface->render(simulationManager->getParticleData());

        simWindow.swapBuffers();
    }

    controlWindow.stop();

    graphicsInterface.reset();
    simulationManager.reset();
    stateReconstructionController.reset();
}
