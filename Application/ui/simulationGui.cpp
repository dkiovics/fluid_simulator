#include "simulationGui.h"
#include <engine/renderEngine.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include "../gfx/gfx2D/simulationGfx2D.hpp"
#include "../gfx/gfx3D/simulationGfx3D.h"
#include "../gfx/gfxInterface.hpp"
#include "manager/simulationManager.h"
#include <chrono>

using namespace genericfsim::manager;
using CellType = genericfsim::macgrid::MacGridCell::CellType;

void startSimulatorGui() {
    const int initialScreenWidth = 1800;
    const int initialScreenHeight = 1300;
    const float aspectRatio = 16.0f / 9.0f;

    std::shared_ptr<renderer::RenderEngine> engine(new renderer::RenderEngine(initialScreenWidth, initialScreenHeight, "Fluid Simulator"));
    GLFWwindow* window = engine->getWindow();
    glfwMakeContextCurrent(window);
    //glfwSwapInterval(1);

    SimulationConfig config2D;
    config2D.averagePressure = 5.66;
    config2D.gridResolution = 3.577;
    config2D.incompressibilityIterationCount = 80;
    config2D.isTopOfContainerSolid = false;
    config2D.particleRadius = 0.051;
    config2D.pressureEnabled = true;
    config2D.pressureK = 1.203;
    config2D.simulatorConfig.flipRatio = 0.923;
    config2D.simulatorConfig.gravity = -176.37;
    config2D.simulatorConfig.gravityEnabled = true;
    config2D.simulatorConfig.pushApartEnabled = true;
    config2D.simulatorConfig.transferType = P2G2PType::FLIP;
    config2D.gridSolverType = SimulationConfig::GridSolverType::BRIDSON;
    config2D.fluidDensity = 1.0;
    config2D.residualTolerance = 1e-6;

    SimulationConfig config3D;
    config3D.averagePressure = 5.43;
    config3D.gridResolution = 1.508;
    config3D.incompressibilityIterationCount = 80;
    config3D.isTopOfContainerSolid = false;
    config3D.particleRadius = 0.182;
    config3D.pressureEnabled = true;
    config3D.pressureK = 1.23;
    config3D.simulatorConfig.flipRatio = 0.85;
    config3D.simulatorConfig.gravity = -250;
    config3D.simulatorConfig.gravityEnabled = true;
    config3D.simulatorConfig.pushApartEnabled = false;
    config3D.simulatorConfig.transferType = P2G2PType::APIC;
    config3D.gridSolverType = SimulationConfig::GridSolverType::BRIDSON;
    config3D.fluidDensity = 1.0;
    config3D.residualTolerance = 1e-6;

    SimulationConfig* config = &config3D;

    const glm::dvec3 dimensions2D(40, 40 / aspectRatio, 3);
    const glm::dvec3 dimensions3D(40, 25, 20);

    int particleNum = 30000;

    std::shared_ptr<SimulationManager> simulationManager = std::make_shared<SimulationManager>(dimensions3D, *config, particleNum, false);

    bool renderer2D = false;
    bool simulation2D = false;
    std::unique_ptr<GfxInterface> simulatorRenderer = std::make_unique<SimulationGfx3D>(engine, simulationManager, glm::ivec2(0, 0), glm::ivec2(1000, 1000), 1000000);

    bool clicked = false;

    engine->mouseButtonCallback.onCallback([&clicked](int button, int action, int mods) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse)
            return;
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            clicked = true;
        }
    });

    simulationManager->startSimulation();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    bool runSimulation = true;
    bool stepSimulation = false;
    bool autodt = true;
    float simdt = 0.02f;

    int selected = 2;
    int cellX = 0;
    int cellY = 0;
    int particleIndex = 0;

    glm::vec3 obstacleColor(0.9f, 0.6f, 0.2f);
    float obstacleRadius = 8;
    glm::vec3 obstacleSize(8, 8, 8);

    float particleSpawnRate = 1600.0f;
    float particleSpawnSpeed = 4.0f;

    int selectedMode = static_cast<int>(config->simulatorConfig.transferType);
    int selectedGridSolver = static_cast<int>(config->gridSolverType);

    bool statisticsWindow = false;

    bool fpsCapTo60 = true;

    auto prevTime = std::chrono::high_resolution_clock::now();

    engine->makeWindowContextcurrent();

    while (!glfwWindowShouldClose(window)) {
        prevTime = std::chrono::high_resolution_clock::now();
        glfwPollEvents();

        const int screenWidth = engine->getScreenWidth();
        const int screenHeight = engine->getScreenHeight();
        const glm::ivec2 simStart = glm::ivec2(0, 0);
        const float tmpWidth = std::min(float(screenWidth), (screenHeight - 150) * aspectRatio);
        const glm::ivec2 simSize = glm::ivec2(tmpWidth, tmpWidth / aspectRatio);
        simulatorRenderer->screenSize = simSize;
        simulatorRenderer->screenStart = simStart;

        float time = glfwGetTime();
        static float lastTime = 0;
        float dt = time - lastTime;
        lastTime = time;

        simulationManager->setAutoDt(autodt);
        simulationManager->setRun(runSimulation);
        simulationManager->setSimulationDt(simdt);
        int prevParticleNum = particleNum = simulationManager->getParticleNum();
        if (stepSimulation)
            simulationManager->stepSimulation();

        simulatorRenderer->handleTimePassed(dt);
        simulatorRenderer->render();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(screenWidth, screenHeight - simSize.y));
        {
            ImGui::Begin("FLIP fluid simulator");

            //Simulation control group
            ImGui::BeginGroup();
            bool newSim = ImGui::Button("Restart sim");
            ImGui::SameLine(0, 30);
            stepSimulation = ImGui::Button("Step sim");
            ImGui::SameLine(0, 30);
            ImGui::Checkbox("2D", &renderer2D);
            ImGui::SameLine(0, 30);
            ImGui::Checkbox("2D sim", &simulation2D);
            ImGui::SameLine(0, 30);
            ImGui::Checkbox("Run", &runSimulation);
            ImGui::SameLine(0, 30);
            ImGui::Checkbox("Auto dt", &autodt);
            ImGui::SameLine(0, 30);
            ImGui::Checkbox("Stop particles", &config->simulatorConfig.stopParticles);
            ImGui::SameLine(0, 30);
            ImGui::Checkbox("FPS cap to 60", &fpsCapTo60);

            {
                if (renderer2D)
                    simulation2D = true;
                bool newSimManager = false;
                if (simulation2D != simulationManager->twoD) {
                    if(simulation2D)
                        config = &config2D;
                    else
                        config = &config3D;
                    selectedMode = static_cast<int>(config->simulatorConfig.transferType);
                    selectedGridSolver = static_cast<int>(config->gridSolverType);
                    simulationManager = std::make_shared<SimulationManager>(simulation2D ? dimensions2D : dimensions3D, *config, particleNum, simulation2D);
                    simulationManager->startSimulation();
                    newSimManager = true;
                }
                if (renderer2D && dynamic_cast<SimulationGfx2D*>(simulatorRenderer.get()) == nullptr) {
                    simulatorRenderer = std::make_unique<SimulationGfx2D>(engine, simulationManager, 1000000);
                    simulationManager->setObstacles(std::vector<std::unique_ptr<Obstacle>>());
                }
                else if (!renderer2D && dynamic_cast<SimulationGfx3D*>(simulatorRenderer.get()) == nullptr) {
                    simulatorRenderer = std::make_unique<SimulationGfx3D>(engine, simulationManager, simStart, simSize, 1000000);
                    simulationManager->setObstacles(std::vector<std::unique_ptr<Obstacle>>());
                }
                else if (newSimManager) {
                    simulatorRenderer->setNewSimulationManager(simulationManager);
                }
            }
            
            ImGui::SetNextItemWidth(screenWidth * 0.45f);
            ImGui::SliderFloat("dt", &simdt, 0.001f, 0.1f);
            ImGui::RadioButton("PIC", &selectedMode, static_cast<int>(P2G2PType::PIC));
            ImGui::SameLine();
            ImGui::RadioButton("FLIP", &selectedMode, static_cast<int>(P2G2PType::FLIP));
            ImGui::SameLine();
            ImGui::RadioButton("APIC", &selectedMode, static_cast<int>(P2G2PType::APIC));
            config->simulatorConfig.transferType = static_cast<P2G2PType>(selectedMode);
            ImGui::SameLine();
            ImGui::Checkbox("Statistics", &statisticsWindow);
            ImGui::SameLine();
            ImGui::Checkbox("Top is solid", &config->isTopOfContainerSolid);
            ImGui::SameLine();
            ImGui::RadioButton("Bridson solver", &selectedGridSolver, static_cast<int>(SimulationConfig::GridSolverType::BRIDSON));
            ImGui::SameLine();
            ImGui::RadioButton("Basic solver", &selectedGridSolver, static_cast<int>(SimulationConfig::GridSolverType::BASIC));
            config->gridSolverType = static_cast<SimulationConfig::GridSolverType>(selectedGridSolver);

            if (config->gridSolverType == SimulationConfig::GridSolverType::BRIDSON) {
                ImGui::SetNextItemWidth(screenWidth * 0.18f);
                ImGui::SliderFloat("density", &config->fluidDensity, 0.1f, 30.0f);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(screenWidth * 0.18f);
                ImGui::SliderFloat("solver tolerance", &config->residualTolerance, 1e-8f, 1e-4f, "%e");
            }

            double simTime = simulationManager->getLastFrameTime();
            ImGui::Text("Rendering: %.1f FPS  Simulation: %.3lfs (%.1lf FPS)", io.Framerate, simTime, 1.0 / simTime);

            ImGui::Checkbox("Particle speed visualizations", &simulatorRenderer->particleSpeedColorEnabled);
            ImGui::SameLine();
            ImGui::ColorEdit3("Color", (float*)&simulatorRenderer->particleColor, ImGuiColorEditFlags_NoInputs);
            if(simulatorRenderer->particleSpeedColorEnabled) {
				ImGui::SameLine();
                ImGui::ColorEdit3("Speed color", (float*)&simulatorRenderer->particleSpeedColor, ImGuiColorEditFlags_NoInputs);
				ImGui::SameLine();
				ImGui::SetNextItemWidth(screenWidth * 0.2f);
				ImGui::SliderFloat("Max speed", &simulatorRenderer->maxParticleSpeed, 0.2f, 100.0f);
            }
            else {
                ImGui::SameLine(0, 30);
            }
            ImGui::ColorEdit3("Gridline color", (float*)&simulatorRenderer->gridLineColor, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::Checkbox("Enable gridlines", &simulatorRenderer->gridlinesEnabled);

            ImGui::SetNextItemWidth(screenWidth * 0.40f);
            ImGui::SliderFloat("Grid resolution", &config->gridResolution, 0.6, 10);
            ImGui::Text("Gridsize x: %d  y: %d  z: %d", simulationManager->getGridSize().x, simulationManager->getGridSize().y, simulationManager->getGridSize().z);
            ImGui::SetNextItemWidth(screenWidth * 0.40f);
            ImGui::SliderInt("Particle count", &particleNum, 1, 100000);
            ImGui::SetNextItemWidth(screenWidth * 0.40f);
            ImGui::SliderFloat("Particle radius", &config->particleRadius, 0.04f, 1.0f);
            ImGui::EndGroup();

            ImGui::SameLine(0, 30);

            //Simulator control group
            ImGui::BeginGroup();
            ImGui::SetNextItemWidth(screenWidth * 0.40f);
            ImGui::SliderInt("N", &config->incompressibilityIterationCount, 1, 600);
            ImGui::SetNextItemWidth(screenWidth * 0.40f);
            ImGui::SliderFloat("Average P", &config->averagePressure, 0.01f, 20.0f);
            ImGui::SetNextItemWidth(screenWidth * 0.40f);
            ImGui::SliderFloat("Pressure k", &config->pressureK, 0.5f, 10.0f);
            ImGui::SetNextItemWidth(screenWidth * 0.40f);
            ImGui::SliderFloat("G", &config->simulatorConfig.gravity, -0.01f, -1000.0f);
            ImGui::SetNextItemWidth(screenWidth * 0.40f);
            ImGui::SliderFloat("Flip", &config->simulatorConfig.flipRatio, 0.0f, 1.0f);

            ImGui::Text("Obstacle:  ");
            ImGui::SameLine();
            if (ImGui::Button("Add sphere")) {
                simulatorRenderer->addSphericalObstacle(obstacleColor, obstacleRadius);
            }
            ImGui::SameLine();
            if (ImGui::Button("Add rectangle")) {
                simulatorRenderer->addRectengularObstacle(obstacleColor, obstacleSize);
            }
            if (SimulationGfx3D* renderer3D = dynamic_cast<SimulationGfx3D*>(simulatorRenderer.get())) {
                ImGui::SameLine();
                if (ImGui::Button("Add particle source")) {
                    renderer3D->addParticleSource(obstacleColor, obstacleRadius, particleSpawnRate, particleSpawnSpeed);
                }
                ImGui::SameLine();
                if (ImGui::Button("Add particle sink")) {
                    renderer3D->addParticleSink(obstacleColor, obstacleRadius);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                simulatorRenderer->removeObstacle();
            }

            ImGui::ColorEdit3("color", (float*)&obstacleColor, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(screenWidth * 0.1f);
            ImGui::SliderFloat("radius", &obstacleRadius, 0.5f, 20.0f);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(screenWidth * 0.08f);
            ImGui::SliderFloat("x", &obstacleSize.x, 1.0f, 40.0f);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(screenWidth * 0.08f);
            ImGui::SliderFloat("y", &obstacleSize.y, 1.0f, 40.0f);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(screenWidth * 0.08f);
            ImGui::SliderFloat("z", &obstacleSize.z, 1.0f, 40.0f);

            if(!renderer2D) {
                ImGui::Text("Particle source:  ");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(screenWidth * 0.1f);
                ImGui::SliderFloat("spawn rate", &particleSpawnRate, 1.0f, 5000.0f);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(screenWidth * 0.1f);
                ImGui::SliderFloat("spawn speed", &particleSpawnSpeed, 0.1f, 10.0f);
                ImGui::Checkbox("Enable particle spawn", &config->simulatorConfig.particleSpawningEnabled);
                ImGui::SameLine();
                ImGui::Checkbox("Enable particle despawn", &config->simulatorConfig.particleDespawningEnabled);
                ImGui::SameLine();
            }

            ImGui::Checkbox("Gravity", &config->simulatorConfig.gravityEnabled);
            ImGui::SameLine(0, 30);
            ImGui::Checkbox("Push apart", &config->simulatorConfig.pushApartEnabled);
            ImGui::SameLine(0, 30);
            ImGui::Checkbox("Pressure", &config->pressureEnabled);

            if (SimulationGfx2D* renderer2D = dynamic_cast<SimulationGfx2D*>(simulatorRenderer.get())) {
                ImGui::SameLine(0, 30);
                ImGui::RadioButton("Cell", &selected, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Particle", &selected, 1);
                ImGui::SameLine();
                ImGui::RadioButton("None", &selected, 2);

                if (clicked) {
                    glm::vec2 click = renderer2D->getMouseGridPos();
                    if (selected == 0) {
                        cellX = click.x;
                        cellY = click.y;
                    }
                    else if (selected == 1) {
                        particleIndex = simulationManager->getParticleIndex(glm::dvec3(click.x, click.y, 1.5));
                    }
                    clicked = false;
                }

                if (selected == 0) {
                    auto& cell = simulationManager->getCellAt(cellX, cellY, 1);
                    ImGui::Text("Cell x: %d  y: %d  type: %s  v.x: %.3lf v.y: %.3lf v.z: %.3lf v2.x: %.3lf v2.y: %.3lf v2.z: %.3lf  p: %.3lf", cellX, cellY,
                        (cell.type == CellType::SOLID ? "solid" : (cell.type == CellType::WATER ? "water" : "air  ")), cell.faces[0].v.load(), cell.faces[1].v.load(), cell.faces[2].v.load(), 
                        cell.faces[0].v2, cell.faces[1].v2, cell.faces[2].v2, cell.avgPNum.load());
                }
                if (selected == 1) {
                    auto& particle = simulationManager->getParticleData(particleIndex);
                    ImGui::Text("Particle index: %d  pos.x: %.3lf pos.y: %.3lf  v.x: %.3lf v.y: %.3lf", particleIndex, particle.pos.x, particle.pos.y, particle.v.x, particle.v.y);
                }
            }

            ImGui::EndGroup();
            ImGui::End();

            if (statisticsWindow) {
                ImGui::Begin("Statistics");
                ImGui::Text("Duration of simulation steps:");
                int p = 0;
                for (auto& s : simulationManager->getStepDuration()) {
                    ImGui::Text("%s: %ld", s.first.c_str(), s.second);
                    p++;
                    if (p % 3 > 0)
                        ImGui::SameLine(0, 10);
                }
                ImGui::End();
            }

            if (newSim) {
                simulationManager->restartSimulation();
            }
            else {
                if(prevParticleNum != particleNum)
                    simulationManager->setParticleNum(particleNum);
                simulationManager->setCalculateParticleSpeeds(simulatorRenderer->particleSpeedColorEnabled);
                simulationManager->setConfig(*config);
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        while (fpsCapTo60 && std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - prevTime).count() < 16666) { }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

