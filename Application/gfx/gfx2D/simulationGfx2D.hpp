#pragma once

#include <engine/renderEngine.h>
#include <geometries/quad.h>
#include <geometries/circle.h>
#include "manager/simulationManager.h"
#include "glm/gtc/matrix_transform.hpp"
#include <algorithm>
#include "../gfxInterface.hpp"


class SimulationGfx2D : public GfxInterface {
private:
	std::shared_ptr<RenderEngine> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulator;
	const int maxParticleNum;

	int gridWidth;
	int gridHeight;

	glm::vec3 prevParticleColor;

	glm::mat4 M;
	float magnification = 1;
	glm::vec3 magPos = glm::vec3(0.5, 0.5, 0);
	glm::vec2 mousePos = glm::vec2(0.5, 0.5);
	bool mouseValid = true;

	unsigned int basicParticleProgram;
	unsigned int gridProgram;
	std::unique_ptr<Circle> particlesGfx;
	std::unique_ptr<Quad> gridlinesGfx;

	std::vector<glm::dvec2> obstacleBoundingBoxes;
	std::vector<std::unique_ptr<Geometry>> obstacleGfx;

	int draggedObstacle = -1;
	glm::vec2 prevMousePos;

	bool topIsSolid = false;

	int mouseCallbackNum;
	int mouseButtonCallbackNum;
	int scrollCallbackNum;

	bool particleSpeedColorWasEnabled = false;

private:

	void updateParticles() {
		auto particles = simulator->getParticleGfxData();
		int particleNum = particles.size();
		if (!particleSpeedColorEnabled && (particleColor != prevParticleColor || particleSpeedColorWasEnabled)) {
			particleSpeedColorWasEnabled = false;
			for (int p = 0; p < particleNum; p++) {
				particlesGfx->colorAt(p) = glm::vec4(particleColor, 1.0);
			}
			particlesGfx->updateColor(particleNum);
		}
		const float maxSpeedInv = 1.0f / maxParticleSpeed;
		for (int p = 0; p < particleNum; p++) {
			particlesGfx->positionAt(p) = glm::vec3(particles[p].pos.x, particles[p].pos.y, 1);
			if (particleSpeedColorEnabled) {
				float s = std::min(particles[p].v * maxSpeedInv, 1.0f);
				s = std::pow(s, 0.3f);
				particlesGfx->colorAt(p) = glm::vec4((particleColor * (1.0f - s)) + (particleSpeedColor * s), 1);
			}
		}
		particlesGfx->updatePosition(particleNum);
		if (particleSpeedColorEnabled) {
			particlesGfx->updateColor(particleNum);
			particleSpeedColorWasEnabled = true;
		}
	}

	void mouseCallback(double x, double y) {
		int totalH = engine->getScreenHeight();
		mousePos = glm::vec2((x - screenStart.x) / screenSize.x, ((totalH - y) - screenStart.y) / screenSize.y);
		if (mousePos.x < 0 || mousePos.x > 1 || mousePos.y < 0 || mousePos.y > 1) {
			mouseValid = false;
			draggedObstacle = -1;
			return;
		}
		mouseValid = true;
		glm::vec2 d = mousePos - prevMousePos;
		prevMousePos = mousePos;
		d.x *= simulator->getDimensions().x;
		d.y *= simulator->getDimensions().y;
		if (draggedObstacle != -1) {
			auto obstacles = simulator->getObstacles();
			obstacles[draggedObstacle]->setNewPos(obstacles[draggedObstacle]->pos + glm::dvec3(d.x, d.y, 0));
			obstacleGfx[draggedObstacle]->positionAt(0) = glm::vec3(obstacles[draggedObstacle]->pos.x, obstacles[draggedObstacle]->pos.y, 0);
			obstacleGfx[draggedObstacle]->updatePosition();
			simulator->setObstacles(std::move(obstacles));
		}
	}

	void mouseButtonCallback(int button, int action, int mods) {
		if (!mouseValid)
			return;
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			auto obstacles = simulator->getObstacles();
			for (int p = 0; p < obstacleBoundingBoxes.size(); p++) {
				glm::vec2 mPos = glm::vec2(mousePos.x * simulator->getDimensions().x, mousePos.y * simulator->getDimensions().y);
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
		glm::dvec3 dimensions = simulator->getDimensions();
		if (magnification == 1)
			magPos = glm::vec3(mousePos.x * dimensions.x, mousePos.y * dimensions.y, 0);
		else
			magPos = glm::vec3((mousePos.x - 0.5f) * dimensions.x / magnification + magPos.x, (mousePos.y - 0.5f) * dimensions.y / magnification + magPos.y, 0);
		magnification += amount;
		if (magnification < 1)
			magnification = 1;
		magPos.x = std::clamp(double(magPos.x), dimensions.x / magnification * 0.5f - 0.1f, dimensions.x - dimensions.x / magnification * 0.5f + 0.1f);
		magPos.y = std::clamp(double(magPos.y), dimensions.y / magnification * 0.5f - 0.1f, dimensions.y - dimensions.y / magnification * 0.5f + 0.1f);
		M = glm::mat4(1);
		M = glm::scale(M, glm::vec3(2.0f / dimensions.x * magnification, 2.0f / dimensions.y * magnification, 1));
		M = glm::translate(M, -magPos);
	}

	void handleSimSpecChanged() {
		int gridWidth = simulator->getGridSize().x;
		int gridHeight = simulator->getGridSize().y;
		if (this->gridHeight != gridHeight || this->gridWidth != gridWidth || topIsSolid != simulator->getConfig().isTopOfContainerSolid) {
			topIsSolid = simulator->getConfig().isTopOfContainerSolid;
			this->gridWidth = gridWidth;
			this->gridHeight = gridHeight;
			int squareNum = gridWidth * gridHeight;
			std::vector<glm::vec3> gridPosition(squareNum);
			std::vector<glm::vec4> gridColor;
			gridColor.reserve(squareNum);

			auto config = simulator->getConfig();
			float cellD = 1 / config.gridResolution;

			for (int y = 0; y < gridHeight; y++) {
				for (int x = 0; x < gridWidth; x++) {
					gridPosition[x + y * gridWidth] = glm::vec3(x * cellD + cellD * 0.5, y * cellD + cellD * 0.5, 0);
					gridColor.push_back(x == 0 || y == 0 || x == gridWidth - 1 || (y == gridHeight - 1 && config.isTopOfContainerSolid) ? glm::vec4(1, 0, 0, 1) : glm::vec4(0, 0, 0, 1));
				}
			}
			gridlinesGfx = std::make_unique<Quad>(squareNum, gridPosition, gridColor, engine, cellD, cellD);
		}
	}

public:

	SimulationGfx2D(std::shared_ptr<RenderEngine> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulator, int maxParticleNum)
		: engine(engine), simulator(simulator), maxParticleNum(maxParticleNum) {
		prevParticleColor = particleColor;
		std::vector<glm::vec3> positions(maxParticleNum);
		std::vector<glm::vec4> colors(maxParticleNum);
		for (auto& c : colors) {
			c = glm::vec4(particleColor, 1.0);
		}
		particlesGfx = std::make_unique<Circle>(maxParticleNum, positions, colors, engine, 1.0f, 20);

		basicParticleProgram = engine->createGPUProgram("shaders/basic.vs", "shaders/basic.fs");
		gridProgram = engine->createGPUProgram("shaders/eulerGrid.vs", "shaders/eulerGrid.fs");

		glm::dvec3 dimensions = simulator->getDimensions();

		mouseCallbackNum = engine->mouseCallback.onCallback([this](double x, double y) { mouseCallback(x, y); });
		scrollCallbackNum = engine->scrollCallback.onCallback([this](double x, double y) { scrollCallback(x, y); });
		mouseButtonCallbackNum = engine->mouseButtonCallback.onCallback([this](int a, int b, int c) { mouseButtonCallback(a, b, c); });

		M = glm::mat4(1);
		M = glm::translate(M, glm::vec3(-1, -1, 0));
		M = glm::scale(M, glm::vec3(2.0f / dimensions.x * magnification, 2.0f / dimensions.y * magnification, 1));
		handleSimSpecChanged();
	}

	void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> manager) override {
		simulator = manager;
		obstacleBoundingBoxes.clear();
		obstacleGfx.clear();
		handleSimSpecChanged();
	}

	//returns the position on the grid including the magnification
	glm::vec2 getMouseGridPos() {
		if (magnification == 1)
			return glm::vec2(mousePos.x * gridWidth, mousePos.y * gridHeight);
		else
			return glm::vec2((mousePos.x - 0.5f) * gridWidth / magnification + magPos.x, (mousePos.y - 0.5f) * gridHeight / magnification + magPos.y);
	}

	void render() override {
		handleSimSpecChanged();
		updateParticles();
		glDisable(GL_DEPTH_TEST);
		engine->setViewport(screenStart.x, screenStart.y, screenSize.x, screenSize.y);
		engine->clearViewport(glm::vec4(0, 0, 0, 0), GL_COLOR_BUFFER_BIT);
		engine->activateGPUProgram(basicParticleProgram);
		engine->setUniformMat4(basicParticleProgram, "M", M);
		engine->setUniformFloat(basicParticleProgram, "scale", 1.0f);
		gridlinesGfx->draw();
		engine->setUniformFloat(basicParticleProgram, "scale", simulator->getConfig().particleRadius);
		particlesGfx->draw(simulator->getParticleNum());
		engine->setUniformFloat(basicParticleProgram, "scale", 1.0f);
		for (auto& o : obstacleGfx) {
			o->draw();
		}
		if (gridlinesEnabled) {
			engine->activateGPUProgram(gridProgram);
			engine->renderWireframeOnly(true);
			engine->setUniformVec4(gridProgram, "color", glm::vec4(gridLineColor, 1));
			engine->setUniformMat4(gridProgram, "M", M);
			gridlinesGfx->draw();
			engine->renderWireframeOnly(false);
		}

	}

	//pos between [0, 1]
	void addSphericalObstacle(glm::vec3 color, float r) override {
		std::unique_ptr<genericfsim::manager::SphericalObstacle> obstacle = 
			std::make_unique<genericfsim::manager::SphericalObstacle>(r, glm::dvec3(simulator->getDimensions().x * 0.5, simulator->getDimensions().y * 0.5, 0));
		obstacleBoundingBoxes.push_back(glm::dvec2(r * 2, r * 2));
		std::vector<glm::vec3> positions = { glm::vec3(obstacle->pos.x, obstacle->pos.y, 0) };
		std::vector<glm::vec4> colors = { glm::vec4(color, 1) };
		obstacleGfx.push_back(std::make_unique<Circle>(1, positions, colors, engine, r, 80));

		auto obstacles = simulator->getObstacles();
		obstacles.push_back(std::move(obstacle));
		simulator->setObstacles(std::move(obstacles));
	}

	//pos between [0, 1]
	void addRectengularObstacle(glm::vec3 color, glm::vec3 size) override {
		std::unique_ptr<genericfsim::manager::RectengularObstacle> obstacle =
			std::make_unique<genericfsim::manager::RectengularObstacle>(glm::dvec3(size.x, size.y, size.z), glm::dvec3(simulator->getDimensions().x * 0.5, simulator->getDimensions().y * 0.5, 0));
		obstacleBoundingBoxes.push_back(glm::dvec2(size.x, size.y));
		std::vector<glm::vec3> positions = { glm::vec3(obstacle->pos.x, obstacle->pos.y, 0) };
		std::vector<glm::vec4> colors = { glm::vec4(color, 1) };
		obstacleGfx.push_back(std::make_unique<Quad>(1, positions, colors, engine, size.x, size.y));

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
			obstacleGfx.pop_back();
			draggedObstacle = -1;
		}
	}

	~SimulationGfx2D() override {
		engine->deleteGPUProgram(basicParticleProgram);
		engine->deleteGPUProgram(gridProgram);
		engine->mouseCallback.removeCallbackFunction(mouseCallbackNum);
		engine->mouseButtonCallback.removeCallbackFunction(mouseButtonCallbackNum);
		engine->scrollCallback.removeCallbackFunction(scrollCallbackNum);
	}

};
