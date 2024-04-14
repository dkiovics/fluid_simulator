#pragma once

#include <engine/renderEngine.h>
#include <geometries/basicGeometries.h>
#include <engineUtils/object.h>
#include "manager/simulationManager.h"
#include "glm/gtc/matrix_transform.hpp"
#include <algorithm>
#include "../gfxInterface.hpp"
#include <engineUtils/camera2D.hpp>


class SimulationGfx2D : public GfxInterface {
private:
	std::shared_ptr<renderer::RenderEngine> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulator;
	const int maxParticleNum;

	int gridWidth;
	int gridHeight;

	glm::vec3 prevParticleColor;

	/*glm::mat4 P;
	float magnification = 1;
	glm::vec3 magPos = glm::vec3(0.5, 0.5, 0);*/
	//In normalized device coordinates
	glm::vec2 mousePos = glm::vec2(0, 0);
	bool mouseValid = true;

	std::shared_ptr<renderer::Camera2D> camera;

	std::shared_ptr<renderer::ShaderProgram> basicArrayProgram;
	std::shared_ptr<renderer::ShaderProgram> basicArrayProgramSingleColor;
	std::shared_ptr<renderer::ShaderProgram> basicProgram;

	std::unique_ptr<renderer::Object2D<renderer::BasicGeometryArray>> gridlinesGfx;
	std::unique_ptr<renderer::Object2D<renderer::BasicGeometryArray>> particlesGfx;

	std::vector<glm::dvec2> obstacleBoundingBoxes;
	std::vector<std::unique_ptr<renderer::Object2D<renderer::Geometry>>> obstacleGfxObjects;

	int draggedObstacle = -1;
	glm::vec2 prevMousePos;

	bool topIsSolid = false;

	int mouseCallbackNum;
	int mouseButtonCallbackNum;
	int scrollCallbackNum;

	bool particleSpeedColorWasEnabled = false;

private:

	void updateParticles() {
		particlesGfx->setScale(glm::vec3(simulator->getConfig().particleRadius, simulator->getConfig().particleRadius, 1));
		auto particles = simulator->getParticleGfxData();
		int particleNum = particles.size();
		particlesGfx->drawable->setActiveInstanceNum(particleNum);
		if (!particleSpeedColorEnabled && (particleColor != prevParticleColor || particleSpeedColorWasEnabled)) {
			particleSpeedColorWasEnabled = false;
			for (int p = 0; p < particleNum; p++) {
				particlesGfx->drawable->setColor(p, glm::vec4(particleColor, 1.0));
			}
		}
		const float maxSpeedInv = 1.0f / maxParticleSpeed;
		for (int p = 0; p < particleNum; p++) {
			particlesGfx->drawable->setOffset(p, glm::vec4(particles[p].pos.x, particles[p].pos.y, 1, 0));
			if (particleSpeedColorEnabled) {
				float s = std::min(particles[p].v * maxSpeedInv, 1.0f);
				s = std::pow(s, 0.3f);
				particlesGfx->drawable->setColor(p, glm::vec4((particleColor * (1.0f - s)) + (particleSpeedColor * s), 1));
			}
		}
		if (particleSpeedColorEnabled) {
			particleSpeedColorWasEnabled = true;
		}
	}

	void mouseCallback(double x, double y) {
		mousePos = glm::vec2((x - screenStart.x) / screenSize.x * 2.0f - 1.0f, (screenStart.y - y) / screenSize.y * 2.0f + 1.0f);
		if (mousePos.x < -1 || mousePos.x > 1 || mousePos.y < -1 || mousePos.y > 1) {
			mouseValid = false;
			draggedObstacle = -1;
			return;
		}
		mouseValid = true;
		glm::vec2 d = camera->getWorldPosition(mousePos) - camera->getWorldPosition(prevMousePos);
		prevMousePos = mousePos;
		if (draggedObstacle != -1) {
			auto obstacles = simulator->getObstacles();
			obstacles[draggedObstacle]->setNewPos(obstacles[draggedObstacle]->pos + glm::dvec3(d.x, d.y, 0));
			obstacleGfxObjects[draggedObstacle]->setPosition(glm::vec4(obstacles[draggedObstacle]->pos.x, obstacles[draggedObstacle]->pos.y, 0, 1));
			simulator->setObstacles(std::move(obstacles));
		}
	}

	void mouseButtonCallback(int button, int action, int mods) {
		if (!mouseValid)
			return;
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			auto obstacles = simulator->getObstacles();
			for (int p = 0; p < obstacleBoundingBoxes.size(); p++) {
				glm::vec2 mPos = getMouseGridPos();
				if (fabs(mPos.x - obstacles[p]->pos.x) < obstacleBoundingBoxes[p].x * 0.5f && fabs(mPos.y - obstacles[p]->pos.y) < obstacleBoundingBoxes[p].y * 0.5f)
					draggedObstacle = p;
			}
		}
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
			draggedObstacle = -1;
		}
	}

	void scrollCallback(double xoffset, double yoffset) {
		if (!mouseValid)
			return;
		float amount = (yoffset > 0) ? 0.5f : -0.5f;
		camera->magnify(mousePos, amount);
	}

	void handleSimSpecChanged() {
		int gridWidth = simulator->getGridSize().x;
		int gridHeight = simulator->getGridSize().y;
		if (this->gridHeight != gridHeight || this->gridWidth != gridWidth || topIsSolid != simulator->getConfig().isTopOfContainerSolid) {
			topIsSolid = simulator->getConfig().isTopOfContainerSolid;
			this->gridWidth = gridWidth;
			this->gridHeight = gridHeight;
			int squareNum = gridWidth * gridHeight;
			std::vector<glm::vec4> gridPosition(squareNum);
			std::vector<glm::vec4> gridColor;
			gridColor.reserve(squareNum);

			auto config = simulator->getConfig();
			float cellD = 1 / config.gridResolution;

			for (int y = 0; y < gridHeight; y++) {
				for (int x = 0; x < gridWidth; x++) {
					gridPosition[x + y * gridWidth] = glm::vec4(x * cellD + cellD * 0.5, y * cellD + cellD * 0.5, 0, 0);
					gridColor.push_back(x == 0 || y == 0 || x == gridWidth - 1 || (y == gridHeight - 1 && config.isTopOfContainerSolid) ? glm::vec4(1, 0, 0, 1) : glm::vec4(0, 0, 0, 1));
				}
			}
			gridlinesGfx->drawable->setMaxInstanceNum(squareNum, std::move(gridPosition), std::move(gridColor));
			gridlinesGfx->drawable->setActiveInstanceNum(squareNum);
			gridlinesGfx->setScale(glm::vec3(cellD, cellD, 1));
		}
	}

public:

	SimulationGfx2D(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulator, int maxParticleNum)
		: engine(engine), simulator(simulator), maxParticleNum(maxParticleNum) {
		prevParticleColor = particleColor;
		std::vector<glm::vec4> positions(maxParticleNum);
		std::vector<glm::vec4> colors(maxParticleNum);
		for (auto& c : colors) {
			c = glm::vec4(particleColor, 1.0);
		}

		auto dimensions = simulator->getDimensions();
		camera = std::make_shared<renderer::Camera2D>(glm::vec2(dimensions.x, dimensions.y), glm::vec2(dimensions.x, dimensions.y) * 0.5f);

		basicArrayProgram = std::make_shared<renderer::ShaderProgram>("shaders/basic2DArray.vs", "shaders/basic2D.fs", engine);
		basicArrayProgramSingleColor = std::make_shared<renderer::ShaderProgram>("shaders/basic2DArraySingleColor.vs", "shaders/basic2D.fs", engine);
		basicProgram = std::make_shared<renderer::ShaderProgram>("shaders/basic2D.vs", "shaders/basic2D.fs", engine);
		camera->addProgram({ basicArrayProgram, basicArrayProgramSingleColor, basicProgram });
		camera->setUniformsForAllPrograms();

		auto particlesArray = std::make_shared<renderer::BasicGeometryArray>(std::make_shared<renderer::Circle>(20));
		particlesGfx = std::make_unique<renderer::Object2D<renderer::BasicGeometryArray>>(particlesArray, basicArrayProgram);
		particlesGfx->drawable->setMaxInstanceNum(maxParticleNum, std::move(positions), std::move(colors));
		particlesGfx->setScale(glm::vec3(simulator->getConfig().particleRadius, simulator->getConfig().particleRadius, 1));

		auto gridlinesArray = std::make_shared<renderer::BasicGeometryArray>(std::make_shared<renderer::Square>());
		gridlinesGfx = std::make_unique<renderer::Object2D<renderer::BasicGeometryArray>>(gridlinesArray, basicArrayProgram);

		mouseCallbackNum = engine->mouseCallback.onCallback([this](double x, double y) { mouseCallback(x, y); });
		scrollCallbackNum = engine->scrollCallback.onCallback([this](double x, double y) { scrollCallback(x, y); });
		mouseButtonCallbackNum = engine->mouseButtonCallback.onCallback([this](int a, int b, int c) { mouseButtonCallback(a, b, c); });

		handleSimSpecChanged();
	}

	void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> manager) override {
		simulator = manager;
		camera->init(glm::vec2(simulator->getDimensions()), glm::vec2(simulator->getDimensions()) * 0.5f);
		obstacleBoundingBoxes.clear();
		obstacleGfxObjects.clear();
		handleSimSpecChanged();
	}

	//returns the position on the grid including the magnification, in world coordinates
	glm::vec2 getMouseGridPos() {
		return camera->getWorldPosition(mousePos);
	}

	void render() override {
		engine->enableDepthTest(false);
		engine->clearViewport(glm::vec4(0, 0, 0, 0));
		
		handleSimSpecChanged();
		updateParticles();

		gridlinesGfx->drawable->updateActiveInstanceParams();
		particlesGfx->drawable->updateActiveInstanceParams();

		gridlinesGfx->draw();
		particlesGfx->draw();

		for (auto& o : obstacleGfxObjects) {
			o->draw();
		}
		if (gridlinesEnabled) {
			engine->renderWireframeOnly(true);
			gridlinesGfx->shaderProgram = basicArrayProgramSingleColor;
			gridlinesGfx->diffuseColor = glm::vec4(gridLineColor, 1);
			gridlinesGfx->draw();
			gridlinesGfx->shaderProgram = basicArrayProgram;
			engine->renderWireframeOnly(false);
		}

	}

	//pos between [0, 1]
	void addSphericalObstacle(glm::vec3 color, float r) override {
		std::unique_ptr<genericfsim::manager::SphericalObstacle> obstacle = 
			std::make_unique<genericfsim::manager::SphericalObstacle>(r, glm::dvec3(simulator->getDimensions().x * 0.5, simulator->getDimensions().y * 0.5, 0));
		obstacleBoundingBoxes.push_back(glm::dvec2(r * 2, r * 2));
		auto obstacleObj = std::make_unique<renderer::Object2D<renderer::Geometry>>(std::make_unique<renderer::Circle>(80), basicProgram);
		obstacleObj->setPosition(glm::vec4(obstacle->pos.x, obstacle->pos.y, 0, 1));
		obstacleObj->setScale(glm::vec3(r, r, 1));
		obstacleObj->diffuseColor = glm::vec4(color, 1);
		obstacleGfxObjects.push_back(std::move(obstacleObj));

		auto obstacles = simulator->getObstacles();
		obstacles.push_back(std::move(obstacle));
		simulator->setObstacles(std::move(obstacles));
	}

	//pos between [0, 1]
	void addRectengularObstacle(glm::vec3 color, glm::vec3 size) override {
		std::unique_ptr<genericfsim::manager::RectengularObstacle> obstacle =
			std::make_unique<genericfsim::manager::RectengularObstacle>(glm::dvec3(size.x, size.y, size.z), glm::dvec3(simulator->getDimensions().x * 0.5, simulator->getDimensions().y * 0.5, 0));
		obstacleBoundingBoxes.push_back(glm::dvec2(size.x, size.y));
		auto obstacleObj = std::make_unique<renderer::Object2D<renderer::Geometry>>(std::make_unique<renderer::Square>(), basicProgram);
		obstacleObj->setPosition(glm::vec4(obstacle->pos.x, obstacle->pos.y, 0, 1));
		obstacleObj->setScale(glm::vec3(size.x, size.y, 1));
		obstacleObj->diffuseColor = glm::vec4(color, 1);
		obstacleGfxObjects.push_back(std::move(obstacleObj));

		auto obstacles = simulator->getObstacles();
		obstacles.push_back(std::move(obstacle));
		simulator->setObstacles(std::move(obstacles));
	}

	void removeObstacle() override {
		if (obstacleBoundingBoxes.size() > 0) {
			obstacleBoundingBoxes.pop_back();
			auto obstacles = simulator->getObstacles();
			obstacles.pop_back();
			simulator->setObstacles(std::move(obstacles));
			obstacleGfxObjects.pop_back();
			draggedObstacle = -1;
		}
	}

	~SimulationGfx2D() override {
		engine->mouseCallback.removeCallbackFunction(mouseCallbackNum);
		engine->mouseButtonCallback.removeCallbackFunction(mouseButtonCallbackNum);
		engine->scrollCallback.removeCallbackFunction(scrollCallbackNum);
	}

};
